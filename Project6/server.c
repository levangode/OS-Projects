#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "dirent.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include "vector.h"


#define BACKLOG 128
#define BUFFER_SIZE 2048
#define MAX_CONFIGFILE_SIZE 5000
#define SUCCESS 1
#define PORTS_LOWER_BOUND 1024
#define PORTS_UPPER_BOUND 65535
#define HASH_MAX_SIZE 1024
#define MAX_URL_LENGTH 512

struct virtual_server{
	char* vhost;
	char* documentroot;
	char* cgi_bin;
	char* ip;
	char* port;
	char* logg;
	struct sockaddr_in my_addr;
	int socket_fd;
};

void handle_request(struct virtual_server*, char*, int, struct sockaddr_in*);
void generate_files(int, DIR*, char*, char*);
void send_file(char*, int, char*, char*);
void return_bad_request(int, char*);
char* contains_range_header(char*);
void send_file_range(int, char*, int, int);
bool check_cache(char*, char*);
void send_not_modified(int, char*);
void send_ok(char*, char*, int, char*, char*);
bool keep_alive(char*);
void receive_and_respond(struct virtual_server*, int, char*, bool*, struct sockaddr_in*);
char* extract_header_token(char*, char*);
void read_config_file(char*);
void* launch_server(void* arg);
int poll_and_serve(void*);
void cgi(char* buffer,char* path,char* method,int client_fd);

/* Returns response to client with a success code, adding cache control and etags to it
 * Also does some logging staff */
void send_ok(char* generated, char* path, int client_fd, char* type, char* logBuff){
	sprintf(logBuff+strlen(logBuff), "%s ", "200");
	sprintf(generated, "HTTP/1.1 200 OK\r\n");
	send(client_fd, generated, strlen(generated), 0);
	sprintf(generated, "Content-Type: %s\r\n", type);
	send(client_fd, generated, strlen(generated), 0);
	sprintf(generated, "Cache-Control: max-age=5\r\n");
	send(client_fd, generated, strlen(generated), 0);
	struct stat file_stat;
	if(strlen(path) == 0){
		stat("/", &file_stat);
	} else {
		stat(path, &file_stat);
	}
	char current_hash[HASH_MAX_SIZE];
	sprintf(current_hash, "%d/%d/%d", (int)file_stat.st_ino, (int)file_stat.st_mtime, (int)file_stat.st_size);
	sprintf(generated, "ETag: %s\r\n", current_hash);
	send(client_fd, generated, strlen(generated), 0);
}

/* Returns response to client with a not modified code */
void send_not_modified(int client_fd, char* logBuff){
	sprintf(logBuff+strlen(logBuff), "%s ", "304");
	char generated[BUFFER_SIZE];
	sprintf(generated, "HTTP/1.1 304 Not Modified\r\n");
	send(client_fd, generated, strlen(generated), 0);
}

/* Checks the header, if the cache control is enabled, && the hash tags are equal
 * returns true */
bool check_cache(char* buff, char* path){
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);
	char* etag = strstr(tmpbuff, "If-None-Match:");
	if(etag == NULL){
		return false;
	}
	strtok(etag, " ");
	char* old_hash = strtok(NULL, " \r\n");
	//compare old_hash and current_hash
	struct stat file_stat;
	if(strlen(path) == 0){
		stat("/", &file_stat);
	} else {
		stat(path, &file_stat);
	}
	char current_hash[HASH_MAX_SIZE];
	sprintf(current_hash, "%d/%d/%d", (int)file_stat.st_ino, (int)file_stat.st_mtime, (int)file_stat.st_size);
	//this hash generation triple got from stack overflow

	if(strcmp(old_hash, current_hash) == 0){
		return true;
	}
	return false;
}

/* Parses the range header and defines byte range to be sent */
void send_file_range(int client_fd, char* range, int fd, int size){
	strtok(range, "=");
	char* bytes = strtok(NULL, "\r\n");
	char* first = strtok(bytes, "-");
	char* second = strtok(NULL, "\n");
	off_t start = (off_t)atoi(first);
	if(second == NULL){
		int s = size - (int)start + 1;
		printf("%d\n", (int)start);
		printf("%d\n", s);
		sendfile(client_fd, fd, &start, (size_t)s);
	} else {
		off_t end = (off_t)atoi(second);
		int s = end - start + 1;
		printf("%d\n", (int)start);
		printf("%d\n", s);
		sendfile(client_fd, fd, &start, (size_t)s);
	}

}

/* Returns pointer to range header if range header is present */
char* contains_range_header(char* buff){
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);
	char* range = strstr(tmpbuff, "Range: ");
	if(range == NULL){
		return NULL;
	}
	return range;	//points to-> Range: bytes
}

/* Returns response to client with not found code
 * also does some logging staff */
void return_bad_request(int client_fd, char* logBuff){
	sprintf(logBuff+strlen(logBuff), "%s ", "404");
	char generated[BUFFER_SIZE];
	memset(generated, '\0', BUFFER_SIZE);
	sprintf(generated, "HTTP/1.1 404 Not Found\r\n");
	send(client_fd, generated, strlen(generated), 0);
	char* tmp = "<h1>404 Not Found</h1>\nthe requested file doesn't exist on this server";
	sprintf(generated, "Content-Type: text/html\r\n");
	send(client_fd, generated, strlen(generated), 0);
	sprintf(generated, "Content-Length: %d\r\n\n", (int)strlen(tmp));
	sprintf(logBuff+strlen(logBuff), "%d ", (int)strlen(tmp));
	send(client_fd, generated, strlen(generated), 0);
	send(client_fd, tmp, strlen(tmp), 0);
}

/* Sends file requested by path to client. (If the file is available)
 * Also checks the range of bytes the client requested and responds relatively */
void send_file(char* path, int client_fd, char* buff, char* logBuff){
	char* type;	//content type that goes into response
	char tmppath[MAX_URL_LENGTH];
	strcpy(tmppath, path);
	strtok(tmppath, ".");
	char* ext = strtok(NULL, "\0");	//extract the file extension
	assert(ext != NULL);

	if(strcmp(ext, "jpg") == 0){
		type = "image/jpg";
	} else if(strcmp(ext, "mp4") == 0){
		type = "video/mp4";
	} else if(strcmp(ext, "html") == 0) {
		type = "text/html";
	} else {
		assert(false);
	}
	char generated[BUFFER_SIZE];
	memset(generated, '\0', BUFFER_SIZE);

	send_ok(generated, path, client_fd, type, logBuff);
	int fd = -1;
	fd = open(path, O_RDONLY);
	if(fd == -1){
		perror("Couldn't open file to send");
		exit(SUCCESS);
	}
	FILE* file = fdopen(fd, "r");
	fseek(file, 0, SEEK_END);
	int size = ftell(file);	//rewind to get file size
	off_t offset = 0;
	sprintf(generated, "Content-Length: %d\r\n\n", size);
	sprintf(logBuff+strlen(logBuff), "%d ", size);
	send(client_fd, generated, strlen(generated), 0);
	char* range = contains_range_header(buff);
	if(range != NULL){
		send_file_range(client_fd, range, fd, size);
	} else {
		sendfile(client_fd, fd, &offset, size);
	}
}

/* Generates list of files existing in the current directory
 * make html list of them and sends to client 
 * + some logging stuff */
void generate_files(int client_fd, DIR* dir, char* path, char* logBuff){
	if(dir == NULL){
		perror("Couldn't open directory");
		exit(-1);
	}
	struct dirent *entry;	
	char generated[BUFFER_SIZE];
	char links[BUFFER_SIZE];
	memset(links, '\0', BUFFER_SIZE);
	send_ok(generated, path, client_fd, "text/html", logBuff);
	sprintf(links+strlen(links), "<html>\n<body>\r\n");
	while(true){
		entry = readdir(dir);
		if(entry == NULL) break;
		if(strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0){	//exclude parent directories
			char* name = entry->d_name;		
			sprintf(links+strlen(links), "<a href='%s/%s'>%s</a><br>\n", path, name, name);
		}
	}
	sprintf(links+strlen(links), "</body>\n</html>\r\n");
	sprintf(generated, "Content-Length: %d\r\n\n", (int)strlen(links));
	sprintf(logBuff+strlen(logBuff), "%d ", (int)strlen(links));

	send(client_fd, generated, strlen(generated), 0);
	send(client_fd, links, strlen(links), 0);
	closedir(dir);
}

bool is_cgi(char* method,char* path){
	if(strcasecmp(method,"POST")==0)
		return true;
	char tmpPath[1024];
	strcpy(tmpPath,path);
	if(strstr(tmpPath,"?")==NULL){
		return false;
	}
	return true;
}

/* Starts making the log script, which then will be written to file */
void make_log(char* buff, struct virtual_server* server, char* path, char* logBuff, struct sockaddr_in* client_addr){
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);

	//code from wiki
	time_t current_time;
    char* c_time_string;
    /* Obtain current time. */
    current_time = time(NULL);

    if (current_time == ((time_t)-1))
    {
        (void) fprintf(stderr, "Failure to obtain the current time.\n");
        exit(EXIT_FAILURE);
    }
    /* Convert to local time format. */
    c_time_string = ctime(&current_time);

    if (c_time_string == NULL)
    {
        (void) fprintf(stderr, "Failure to convert the current time.\n");
        exit(EXIT_FAILURE);
    }
    *(c_time_string + strlen(c_time_string) - 1) = '\0';
    //end of code
    sprintf(logBuff, "[%s] ", c_time_string);
   	sprintf(logBuff+strlen(logBuff), "%s ", inet_ntoa(client_addr->sin_addr));
   	//domain name
   	sprintf(logBuff+strlen(logBuff), "%s ", server->vhost);

   	sprintf(logBuff+strlen(logBuff), "/%s ", path);
   	
}

/* Helper function for logging the errors which happen inside the server */
void log_error(struct virtual_server* server, struct sockaddr_in* client_addr, char* error_string){
	char errorBuff[BUFFER_SIZE];

	//code from wiki
	time_t current_time;
    char* c_time_string;
    /* Obtain current time. */
    current_time = time(NULL);
    if (current_time == ((time_t)-1))
    {
        (void) fprintf(stderr, "Failure to obtain the current time.\n");
        exit(EXIT_FAILURE);
    }
    /* Convert to local time format. */
    c_time_string = ctime(&current_time);
    if (c_time_string == NULL)
    {
        (void) fprintf(stderr, "Failure to convert the current time.\n");
        exit(EXIT_FAILURE);
    }
    *(c_time_string + strlen(c_time_string) - 1) = '\0';
    //end of code
    sprintf(errorBuff, "[%s] ", c_time_string);
   	sprintf(errorBuff+strlen(errorBuff), "%s ", inet_ntoa(client_addr->sin_addr));

   	FILE* logfile = fopen(server->logg+1, "a");
	if(logfile == NULL){
		perror("Couldn't open log file\n");
		exit(1);
	}
	fprintf(logfile, "errorlog:\n%s\n", errorBuff);
	fclose(logfile);

}

/* Finishes the log after responding to client and writes it to the log file */
void finish_log(char* logBuff, char* buff, struct virtual_server* server){
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);
	char* user_agent = extract_header_token(tmpbuff, "User-Agent: ");
	sprintf(logBuff+strlen(logBuff), "\"%s\"\n",user_agent);
	free(user_agent);

	FILE* logfile = fopen(server->logg+1, "a");
	if(logfile == NULL){
		perror("Couldn't open log file\n");
		exit(1);
	}
	fprintf(logfile, "accesslog:\n%s\n", logBuff);
	fclose(logfile);
}

void handle_request(struct virtual_server* server, char* buff, int client_fd, struct sockaddr_in* client_addr){
	printf("%s\n", buff);
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);
	char* method = strtok(tmpbuff, " \t\n");	//equals POST or GET
	char* path = strtok(NULL, " \n")+1; // throw "\" away

	char logBuff[BUFFER_SIZE];
	make_log(buff, server, path, logBuff, client_addr);

	/*if(check_cache(buff, path)){
		send_not_modified(client_fd, logBuff);
		return;
	}*/


	char actualPath[strlen(server->documentroot)-1+strlen(path)];
	memcpy(actualPath, server->documentroot+1, strlen(server->documentroot));
	strcat(actualPath, path);

	char indexPath[strlen(actualPath) + strlen("/index.html")];
	memcpy(indexPath, actualPath+1, strlen(actualPath));
	strcat(indexPath, "/index.html");

	printf("actualPath=%s\n", actualPath);

	char cgiPath[strlen(server->cgi_bin) - 1 + strlen(path)];
	memcpy(cgiPath, server->cgi_bin+1, strlen(server->cgi_bin));
	strcat(cgiPath, path);

	if(is_cgi(method, cgiPath)){
		cgi(buff,cgiPath, method, client_fd);
		return;
	}


	if(access(indexPath, F_OK) == 0){
		send_file(indexPath, client_fd, buff, logBuff);
	}

	//case path is directory
	DIR* dir = opendir((char*)actualPath);
	if(dir != NULL){	
		generate_files(client_fd, dir, path, logBuff);
	}
	

	//case path is file
	else if(access((char*)actualPath, F_OK) == 0){
		send_file((char*)actualPath, client_fd, buff, logBuff);
	} else {
		return_bad_request(client_fd, logBuff);
	}
	finish_log(logBuff, buff, server);
}

typedef struct{
	int client_fd;
	struct sockaddr_in client_addr; 

} handler_args;


bool keep_alive(char* buff){
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);
	if(strstr(tmpbuff, "Connection: keep-alive") != NULL){
		return true;
	}
	return false;
}

/* Recieves the request and responds relatively. this is a reccursive function and calls again if
 * the client asked us to keep-alive the connection. (we limit the repeated connection for 5 secs) */
void receive_and_respond(struct virtual_server* server, int client_fd, char* buff, bool* timeout, struct sockaddr_in* client_addr){
	memset(buff, '\0', BUFFER_SIZE);
	int read = recv(client_fd, buff, BUFFER_SIZE, 0);
	if(read <= 0) {
		close(client_fd);
		return;
	}	
	handle_request(server, buff, client_fd, client_addr);
	if(keep_alive(buff)){
		struct timeval t;
		t.tv_sec = 5;
		t.tv_usec = 0; 
		if(!*timeout){
			if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval)) == 0){	//found this option on the web
				*timeout = true;
			} else {
				perror("Couldn't set socket options");
				exit(SUCCESS);
			}
		}
		receive_and_respond(server, client_fd, buff, timeout, client_addr);
	} else {
		close(client_fd);
	}
}

void* handle_client(void* arg){
	struct virtual_server* server = (struct virtual_server*)arg;
	int socket_fd = server->socket_fd;
	struct sockaddr_in client_addr;
	int client_fd = -1;
	char buff[BUFFER_SIZE];

	unsigned sin_size;
	while(true){
		sin_size = sizeof(struct sockaddr_in);
		client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &sin_size);
		if(client_fd == -1){
			perror("Couldn't accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(client_addr.sin_addr));
		bool timeout = false;
		receive_and_respond(server, client_fd, buff, &timeout, &client_addr);
	}
	return NULL;
}

/* Extracts value token given key
 * Returns resulting string.
 * User should maintain the allocated memory after use. */
char* extract_header_token(char* buff, char* token){
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);
	char* token_p = strstr(tmpbuff, token);
	if(*(token_p + strlen(token)) == '\n'){
		return strdup("");
	}
	char* res = strtok(token_p+strlen(token), "\r\n");
	char* ret = strdup(res);
	return ret;
}

/* Reads the config file, parses vhosts and launches threads which will then 
 * start servers for each vhost. */
void read_config_file(char* path_to_config_file){
	char buff[MAX_CONFIGFILE_SIZE];
	FILE* file;
	int length;
	file = fopen(path_to_config_file, "r");
	if(file == NULL){
		perror("Couldn't open config file for reading");
		exit(SUCCESS);
	}
	while(true){
		length = fread(buff, 1, sizeof(buff), file);
		if(length <= 0) break;
	}
	fclose(file);
	char* rest = buff;
	vector servs;	//to keep track of virtual servers whose max number is not defined.
	VectorNew(&servs, sizeof(pthread_t), NULL, 10);
	while (true){
		struct virtual_server* server = malloc(sizeof(struct virtual_server));	//free called from thread.
		server->vhost = extract_header_token(rest, "vhost = ");
		server->documentroot = extract_header_token(rest, "documentroot = ");
		server->cgi_bin = extract_header_token(rest, "cgi-bin = ");
		server->ip = extract_header_token(rest, "ip = ");
		server->port = extract_header_token(rest, "port = ");
		server->logg = extract_header_token(rest, "log = ");
		pthread_t thread;
		pthread_create(&thread, NULL, launch_server, server);
		VectorAppend(&servs, &thread);
		rest=strstr(rest, "\n\n");
		if(rest == NULL) break;
		rest+=2;
	}
	int i;
	for(i=0; i<VectorLength(&servs); i++){
		pthread_join(*(pthread_t*)VectorNth(&servs, i), NULL);
	}
	VectorDispose(&servs);
}

void check_get_post_case(char* method,char* query,char* query_environment,int len,char * contentl_environment){
	if(strncmp("GET",method,3) == 0){
		sprintf(query_environment,"QUERY_STRING=%s",query);
		putenv(query_environment);
	}else{
		sprintf(contentl_environment,"CONTENT_LENGTH=%d",len);
		putenv(contentl_environment);
	}
}


void cgi(char* buffer,char* path,char* method, int client_fd){//read cgi programming manual	
	char* tmpPath = strdup(path);
	char* location = strtok(tmpPath,"?");
	char* query = strtok(NULL,"?");

	printf("location=%s\n", location);
	printf("query=%s\n", query);

	int output[2],input[2];//for cgi pipes
	char* content_length_ptr;
	int content_length;
	if(strcasecmp("POST",method)==0){
		printf("method=%s\n", method);
		content_length_ptr = extract_header_token(buffer,"Content-Length: ");
		content_length = atoi(content_length_ptr);
		char * finder = strstr(buffer,"\r\n\r\n");
		if(finder != NULL){
			finder = finder + 4;
			query = strdup(finder);//to be freed later
		}
		printf("postquery=%s\n", query);
		free(content_length_ptr);
	}

	printf("\n-------im here in cgi--------\n");

	if(pipe(output) < 0 || pipe(input) < 0){
		printf("%s\n", "GG");
		//print error
		return;
	}

	pid_t pid = fork();
	if(pid < 0){
		printf("%s\n", "ZZ");
		//print error
		return;
	}

	char query_environment[256],method_environment[256],contentl_environment[256];


	if(pid == 0){//child case
		dup2(output[1], 1);
  		close(output[0]);
  		dup2(input[0], 0);
  		close(input[1]);

		sprintf(method_environment,"REQUEST_METHOD=%s",method);
		putenv(method_environment);
		check_get_post_case(method,query,query_environment,content_length,contentl_environment);
		//printf("Printing path: %s\n", path);
		//execl("/bin/ls", "ls","-l", NULL);
		execl("cgiTest.cgi", "cgiTest.cgi", (char*)NULL);
		//execl(path,path,NULL);
		exit(0);
	}else{//parent case
		char rec_buff;
		close(output[1]);
  		close(input[0]);
		if(strncmp("POST",method,4)==0){
			//recv(client_fd,&rec_buff,content_length,0);
			//write(input[1],&query,strlen(query));
		}
		while(true){
			ssize_t res = read(output[0],&rec_buff,1);
			if(res <= 0)
				break;
			send(client_fd,&rec_buff,1,0);
		}

		close(output[0]);
		close(input[1]);
		int tmp = 0;
		waitpid(pid,&tmp,0);
		//printf("printing status: %d\n", tmp );
		//wait(tmp);
	}
	printf("\n-------cgi done--------\n");
}

void* launch_server(void* arg){
	struct virtual_server* server = (struct virtual_server*)arg;
	server->socket_fd = -1;
	int success;
	int port = atoi(server->port);
	if(port <= PORTS_LOWER_BOUND || port > PORTS_UPPER_BOUND){
		printf("%s\n", "Illegal port number");
		//log
		exit(SUCCESS);
	}	
	server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server->socket_fd == -1){
		perror("Socket initialization error on socket() call");
		exit(SUCCESS);
	}
	int opt_val = 1;
	if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(SUCCESS);
	}
	//myport and myip to be defined
	server->my_addr.sin_family = AF_INET;	//host byte order
	server->my_addr.sin_port = htons(port);	//short, network byte order
	server->my_addr.sin_addr.s_addr = inet_addr(server->ip);
	memset(&(server->my_addr.sin_zero), '\0', 8);
	//ERRORCHEKING
	if(server->my_addr.sin_addr.s_addr == -1){
		perror( "255.255.255.255 Happened :(" );
		exit(SUCCESS);
	}
	////
	success = bind(server->socket_fd, (struct sockaddr*)&(server->my_addr), sizeof(struct sockaddr));
	//ERRORCHECKING
	if(success == -1){
		perror("Couldn't bind");
		exit(SUCCESS);
	}
	////
	success = listen(server->socket_fd, BACKLOG);
	if(success == -1){
		perror("Could not listen");
		exit(SUCCESS);
	}
	printf("Server Started at port %d\n", port);
	int server_run_status = poll_and_serve(server);
	if (!server_run_status){
		printf("server exited successfully");
	} else {
		perror("server exitted with error");
	}
	close(server->socket_fd);
	free(server);
	return NULL;
}

struct request_info{
	struct virtual_server* v_server;
	int client_fd;
};

void* handle_a_request(void* aux){

	printf("handleing a request on descriptor %d\n", ((struct request_info*)aux)->client_fd);

	struct virtual_server* server = ((struct request_info*)aux)->v_server;
	int client_fd = ((struct request_info*)aux)->client_fd;
	free(aux);

	char buff[BUFFER_SIZE];
	bool timeout = false;

	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);

	receive_and_respond(server, client_fd, buff, &timeout, &addr);



	return NULL;
}

int poll_and_serve(void* server){
	int MAXEVENTS = 512;

	struct virtual_server* v_server = (struct virtual_server*)server;
	int efd; // epool file descriptor
	int sfd = v_server->socket_fd; // socket(server) file descriptor
	struct epoll_event event; // epoll event
  	struct epoll_event *events; // epoll returned event buffer

	efd = epoll_create1 (0);
	if (efd == -1){
		perror ("epoll_create");
		abort ();
	}

	event.data.fd = sfd;
	event.events = EPOLLIN;

	int epoll_error = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
	if (epoll_error == -1){
		perror ("epoll_ctl failed");
		exit(1);
	}

	//allocate buffer for
	events = calloc (MAXEVENTS, sizeof event);

	while (1){
		int i, num_events;

		num_events = epoll_wait(efd, events, MAXEVENTS, -1);

		for(i=0;i<num_events; i++){
			int event_fd = events[i].data.fd;
			//error parsing ignored
			if(event_fd == sfd){
				printf("connection request(1 or more) on server descriptor %d\n", event_fd);
				
				int z=1;
				while(z){// accept all pending requests
					z=0;
					struct sockaddr in_addr;
					socklen_t in_len = sizeof(struct sockaddr);
					int client_fd;
					//char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

					client_fd = accept (sfd, &in_addr, &in_len);

					printf("accepted connection request with descriptor %i\n", event_fd);

					event.data.fd = client_fd;
                  	event.events = EPOLLIN | EPOLLET;
                  	epoll_error = epoll_ctl (efd, EPOLL_CTL_ADD, client_fd, &event);
                  	if (epoll_error == -1){
                      perror ("epoll_ctl failed in poll_and_serve()!");
                    }

				}
			} else {
				printf("data pending on handler socket descriptor %d\n", events[i].data.fd);
				//remove from listeners it no longer requred
				//rest is handled by thread which serves the request
				int client_fd = event_fd;
				epoll_error = epoll_ctl (efd, EPOLL_CTL_DEL, client_fd, &event);
                
                pthread_t t;
                struct request_info* aux = malloc(sizeof(struct request_info));
                aux->v_server = v_server;
                aux->client_fd = client_fd;

                int thread_creat_error = pthread_create(&t, NULL, handle_a_request, aux);
                if(thread_creat_error){
                	perror("creating thread failed in poll_and_serve()!");
                }
			}
		}

	}
	
	return 0;
}


int main(int argc, char *argv[]){
	if(argc	 <= 1){
		printf("%s\n", "You need to specify path to the config file.");
		exit(SUCCESS);
	}
	char* path_to_config_file = argv[1];
	read_config_file(path_to_config_file);

	return 0;
}