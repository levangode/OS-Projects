
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
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include <errno.h>


#define BACKLOG 128
#define BUFFER_SIZE 2048

void handle_request(char*, int);
void generate_files(int, DIR*, char*);
void blank_get(int, char*);
void send_file(char*, int, char*);
void return_bad_request(int);
char* contains_range_header(char*);
void send_file_range(int, char*, int, int);
bool check_cache(char*, char*);
void send_not_modified(int);
void send_ok(char*, char*, int, char*);
bool keep_alive(char*);
void receive_and_respond(int, char*, bool*);

void send_ok(char* generated, char* path, int client_fd, char* type){
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

void send_not_modified(int client_fd){
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
void return_bad_request(int client_fd){
	char generated[1024];
	memset(generated, '\0', 1024);
	sprintf(generated, "HTTP/1.1 404 Not Found\r\n");
	send(client_fd, generated, strlen(generated), 0);
	char* tmp = "<h1>404 Not Found</h1>\nthe requested file doesn't exist on this server";
	sprintf(generated, "Content-Type: text/html\r\n");
	send(client_fd, generated, strlen(generated), 0);
	sprintf(generated, "Content-Length: %d\r\n\n", strlen(tmp));
	send(client_fd, generated, strlen(generated), 0);
	send(client_fd, tmp, strlen(tmp), 0);
}

void send_file(char* path, int client_fd, char* buff){
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
	send_ok(generated, path, client_fd, type);

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
	send(client_fd, generated, strlen(generated), 0);
	char* range = contains_range_header(buff);
	if(range != NULL){
		send_file_range(client_fd, range, fd, size);
	} else {
		sendfile(client_fd, fd, &offset, size);
	}
}

/* Case when request came with "GET / " only */
void blank_get(int client_fd, char* buff){
	if(access("index.html", F_OK) == 0){
		send_file("index.html", client_fd, buff);
	} else {
		char* pwd = getenv("PWD");
		DIR* dir = opendir(pwd);
		generate_files(client_fd, dir, "");
	}
}
/* Generates list of files existing in the current directory
 * make html list of them and sends to client */
void generate_files(int client_fd, DIR* dir, char* path){
	if(dir == NULL){
		perror("Couldn't open directory");
		exit(-1);
	}
	struct dirent *entry;	
	char generated[1024];
	char links[1024];
	memset(links, '\0', 1024);
	send_ok(generated, path, client_fd, "text/html");
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
	send(client_fd, generated, strlen(generated), 0);
	send(client_fd, links, strlen(links), 0);
	closedir(dir);
}

void handle_request(char* buff, int client_fd){
	printf("%s\n", buff);
	char tmpbuff[1024];
	memcpy(tmpbuff, buff, 1024);
	char* method = strtok(tmpbuff, " \t\n");	//equals POST or GET
	char* path = strtok(NULL, " \n")+1; // throw "\" away


	if(check_cache(buff, path)){
		send_not_modified(client_fd);
		return;
	}
	//case blank path
	if(strcmp(path, "") == 0){	
		blank_get(client_fd, buff);
		return;
	}
	//case path is directory
	DIR* dir = opendir(path);
	if(dir != NULL){	
		generate_files(client_fd, dir, path);
	}
	//case path is file
	else if(access(path, F_OK) == 0){
		send_file(path, client_fd, buff);
	} else {
		return_bad_request(client_fd);
	}

	//printf("%s\n", method);
	//printf("%s\n", path);
	//printf("%s\n", http_version);
	//if(strncmp(http_version, "HTTP/1.1", 8) == 0)
	//if(strncmp(method, "GET", 3) == 0)
	//if(strncmp(method, "POST", 4) == 0)	

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

void receive_and_respond(int client_fd, char* buff, bool* timeout){
	printf("%d\n", client_fd);
	memset(buff, '\0', BUFFER_SIZE);
	int read = recv(client_fd, buff, BUFFER_SIZE, 0);
	printf("%s\n", "recieve");
	if(read <= 0) {//an kido racxa moxda
		printf("%s\n", "timeout");
		close(client_fd);
		return;
	}	
	handle_request(buff, client_fd);
	if(keep_alive(buff)){
		printf("%s\n", "keep alive");
		struct timeval t;
		t.tv_sec = 5;
		t.tv_usec = 0; 
		if(!*timeout){
			if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval)) == 0){
				*timeout = true;
				printf("%s\n", "daseta");
			} else {
				printf("%s\n", "ver daseta amchemisam");
				printf("%d,%d,%d,%d,%d,%d,%d,%d,///%d\n", EBADF, EDOM, EINVAL, EISCONN, ENOPROTOOPT, ENOTSOCK, ENOMEM, ENOBUFS, errno);
			}
			
		}
		receive_and_respond(client_fd, buff, timeout);
	} else {
		printf("%s\n", "close");
		close(client_fd);
	}
}
void* handle_client(void* arg){
	int socket_fd = *(int*)arg;
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
		receive_and_respond(client_fd, buff, &timeout);
	}
	return NULL;
}

int main(int argc, char *argv[]){
	int socket_fd = -1;
	int success;
	

	//char* pwd = getenv("PWD");
	
	int port = 4000;
	assert(port > 1024 && port <= 65535);

	struct sockaddr_in my_addr;


	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1){
		perror("Socket initialization error on socket() call");
		exit(1);
	}

	int opt_val = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	//myport and myip to be defined
	my_addr.sin_family = AF_INET;	//host byte order
	my_addr.sin_port = htons(port);	//short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr(MYIP);
	memset(&(my_addr.sin_zero), '\0', 8);

	//ERRORCHEKING
	if(my_addr.sin_addr.s_addr == -1){
		perror( "255.255.255.255 Happened :(" );
	}
	////
	
	success = bind(socket_fd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr));
	//ERRORCHECKING
	if(success == -1){
		perror("Couldn't bind");
		exit(1);
	}
	////
	success = listen(socket_fd, BACKLOG);
	if(success == -1){
		perror("Could not listen");
		exit(1);
	}

	printf("Server Started at port %d\n", port);


	int i;
	for(i=0; i<1024; i++){
		pthread_t thread;
		pthread_create(&thread, NULL, handle_client, &socket_fd);
		pthread_join(thread, NULL);
	}


	close(socket_fd);



	return 0;
}