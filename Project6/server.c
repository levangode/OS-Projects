
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
#include "vector.h"


#define BACKLOG 128
#define BUFFER_SIZE 2048
#define MAX_CONFIGFILE_SIZE 5000

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
void cgi(char* buffer,char* path,char* method,int client_fd);


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
	char current_hash[1024];
	sprintf(current_hash, "%d/%d/%d", (int)file_stat.st_ino, (int)file_stat.st_mtime, (int)file_stat.st_size);

	sprintf(generated, "ETag: %s\r\n", current_hash);
	send(client_fd, generated, strlen(generated), 0);
}

void send_not_modified(int client_fd, char* logBuff){
	sprintf(logBuff+strlen(logBuff), "%s ", "304");
	char generated[1024];
	sprintf(generated, "HTTP/1.1 304 Not Modified\r\n\n");
	send(client_fd, generated, strlen(generated), 0);
}


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
	char current_hash[1024];
	sprintf(current_hash, "%d/%d/%d", (int)file_stat.st_ino, (int)file_stat.st_mtime, (int)file_stat.st_size);
	//this hash generation triple got from stack overflow

	if(strcmp(old_hash, current_hash) == 0){
		return true;
	}

	return false;
}

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

char* contains_range_header(char* buff){
	char tmpbuff[BUFFER_SIZE];
	memcpy(tmpbuff, buff, BUFFER_SIZE);
	char* range = strstr(tmpbuff, "Range: ");
	if(range == NULL){
		return NULL;
	}
	return range;	//points to-> Range: bytes
}
void return_bad_request(int client_fd, char* logBuff){
	sprintf(logBuff+strlen(logBuff), "%s ", "404");
	char generated[1024];
	memset(generated, '\0', 1024);
	sprintf(generated, "HTTP/1.1 404 Not Found\r\n");
	send(client_fd, generated, strlen(generated), 0);
	char* tmp = "<h1>404 Not Found</h1>\nthe requested file doesn't exist on this server";
	sprintf(generated, "Content-Type: text/html\r\n");
	send(client_fd, generated, strlen(generated), 0);
	sprintf(generated, "Content-Length: %d\r\n\n", strlen(tmp));
	sprintf(logBuff+strlen(logBuff), "%d ", strlen(tmp));
	send(client_fd, generated, strlen(generated), 0);
	send(client_fd, tmp, strlen(tmp), 0);
}

void send_file(char* path, int client_fd, char* buff, char* logBuff){
	char* type;	//content type that goes into response
	char tmppath[100];
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

	char generated[1024];
	memset(generated, '\0', 1024);
	
	send_ok(generated, path, client_fd, type, logBuff);


	int fd = -1;

	fd = open(path, O_RDONLY);
	if(fd == -1){
		perror("Couldn't open file to send");
		exit(-1);
	}
	FILE* file = fdopen(fd, "r");
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
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
 * make html list of them and sends to client */
void generate_files(int client_fd, DIR* dir, char* path, char* logBuff){
	if(dir == NULL){
		perror("Couldn't open directory");
		exit(-1);
	}
	struct dirent *entry;	
	char generated[1024];
	char links[1024];
	memset(links, '\0', 1024);
	send_ok(generated, path, client_fd, "text/html", logBuff);
	sprintf(links+strlen(links), "<html>\n<body>\r\n");
	while(true){
		entry = readdir(dir);
		if(entry == NULL) break;
		if(strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0){
			char* name = entry->d_name;		
			sprintf(links+strlen(links), "<a href='%s/%s'>%s</a><br>\n", path, name, name);
		}
	}
	sprintf(links+strlen(links), "</body>\n</html>\r\n");
	sprintf(generated, "Content-Length: %d\r\n\n", strlen(links));
	sprintf(logBuff+strlen(logBuff), "%d ", strlen(links));
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
   	sprintf(logBuff+strlen(logBuff), "/%s ", path);
   	
}
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
	char tmpbuff[1024];
	memcpy(tmpbuff, buff, 1024);
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
			if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval)) == 0){
				*timeout = true;
			} else {
				perror("Couldn't set socket options");
				exit(1);
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


void read_config_file(char* path_to_config_file){
	char buff[MAX_CONFIGFILE_SIZE];
	FILE* file;
	int length;

	file = fopen(path_to_config_file, "r");
	if(file == NULL){
		perror("Couldn't open config file for reading");
	}
	while(true){
		length = fread(buff, 1, sizeof(buff), file);
		if(length <= 0) break;
	}
	fclose(file);
	char* rest = buff;

	
	vector servs;
	VectorNew(&servs, sizeof(pthread_t), NULL, 10);
	while(true){
		struct virtual_server* server = malloc(sizeof(struct virtual_server));
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
		content_length_ptr = extract_header_token(buffer,"Content-Length: ");
		content_length = atoi(content_length_ptr);
		free(content_length_ptr);
	}


	printf("\n-------im here in cgi--------\n");

	if(pipe(output) < 0 || pipe(input) < 0){
		//print error
		return;
	}

	pid_t pid = fork();
	if(pid < 0){
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
		printf("Printing path: %s\n", path);
		execl("/bin/ls", "ls", NULL);
		//execl(path,path,NULL);
		exit(0);
	}else{//parent case
		char rec_buff;
		close(output[1]);
  		close(input[0]);
		if(strncmp("POST",method,4)==0){
			recv(client_fd,&rec_buff,content_length,0);
			write(input[1],&rec_buff,content_length);
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
		printf("printing status: %d\n", tmp );
		//wait(tmp);
	}
	printf("\n-------cgi done--------\n");
}

void* launch_server(void* arg){
	struct virtual_server* server = (struct virtual_server*)arg;
	printf("%s\n", server->vhost);
	printf("%s\n", server->documentroot);
	printf("%s\n", server->cgi_bin);
	printf("%s\n", server->ip);
	printf("%s\n", server->port);
	printf("%s\n", server->logg);
	server->socket_fd = -1;
	int success;
	int port = atoi(server->port);
	assert(port > 1024 && port <= 65535);
	server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server->socket_fd == -1){
		perror("Socket initialization error on socket() call");
		exit(1);
	}
	int opt_val = 1;
	if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	//myport and myip to be defined
	server->my_addr.sin_family = AF_INET;	//host byte order
	server->my_addr.sin_port = htons(port);	//short, network byte order
	server->my_addr.sin_addr.s_addr = inet_addr(server->ip);
	memset(&(server->my_addr.sin_zero), '\0', 8);
	//ERRORCHEKING
	if(server->my_addr.sin_addr.s_addr == -1){
		perror( "255.255.255.255 Happened :(" );
	}
	////
	success = bind(server->socket_fd, (struct sockaddr*)&(server->my_addr), sizeof(struct sockaddr));
	//ERRORCHECKING
	if(success == -1){
		perror("Couldn't bind");
		exit(1);
	}
	////
	success = listen(server->socket_fd, BACKLOG);
	if(success == -1){
		perror("Could not listen");
		exit(1);
	}
	printf("Server Started at port %d\n", port);
	int i;
	pthread_t workers[1024];
	for(i=0; i<1024; i++){
		pthread_create(&workers[i], NULL, handle_client, server);
	}
	for(i=0; i<1024; i++){
		pthread_join(workers[i], NULL);
	}
	close(server->socket_fd);
	free(server);
	return NULL;
}


int main(int argc, char *argv[]){
	if(argc	 <= 1){
		printf("%s\n", "You need to specify path to the config file.");
		exit(-1);
	}
	char* path_to_config_file = argv[1];
	read_config_file(path_to_config_file);

	return 0;
}