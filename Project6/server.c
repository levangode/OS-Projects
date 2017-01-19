
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


#define BACKLOG 128
#define BUFFER_SIZE 2000

void handle_request(char*, int);
void generate_files(int, DIR*);
void blank_get(int, char*);
void send_file(char*, int, char*);
void return_bad_request(int);
char* contains_range_header(char*);
void send_file_range(int, char*, int);

void send_file_range(int client_fd, char* range, int fd){

}

char* contains_range_header(char* buff){
	char tmpbuff[1024];
	memcpy(tmpbuff, buff, strlen(buff));
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

	if(strcmp(ext, "jpg") == 0){
		type = "image/jpg";
	} else if(strcmp(ext, "mp4") == 0){
		type = "video/mp4";
	} else {
		type = "text/html";
	}
	char generated[1024];
	memset(generated, '\0', 1024);
	sprintf(generated, "HTTP/1.1 200 OK\r\n");
	send(client_fd, generated, strlen(generated), 0);
	sprintf(generated, "Content-Type: %s\r\n", type);
	send(client_fd, generated, strlen(generated), 0);
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
		send_file_range(client_fd, range, fd);
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
		generate_files(client_fd, dir);
	}
}
/* Generates list of files existing in the current directory
 * make html list of them and sends to client */
void generate_files(int client_fd, DIR* dir){
	if(dir == NULL){
		perror("Couldn't open directory");
		exit(-1);
	}
	struct dirent *entry;	
	char generated[1024];
	char links[1024];
	memset(links, '\0', 1024);
	sprintf(generated, "HTTP/1.1 200 OK\r\n");
	send(client_fd, generated, strlen(generated), 0);
	sprintf(generated, "Content-Type: text/html\r\n");
	send(client_fd, generated, strlen(generated), 0);
	sprintf(links+strlen(links), "<html>\n<body>\r\n");
	while(true){
		entry = readdir(dir);
		if(entry == NULL) break;
		if(strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0){
			char* name = entry->d_name;		
			sprintf(links+strlen(links), "<a href='%s'>%s</a><br>\n", name, name);
		}
	}
	sprintf(links+strlen(links), "</body>\n</html>\r\n");
	sprintf(generated, "Content-Length: %d\r\n\n", strlen(links));
	send(client_fd, generated, strlen(generated), 0);
	send(client_fd, links, strlen(links), 0);
	closedir(dir);
}

void handle_request(char* buff, int client_fd){
	char tmpbuff[1024];
	memcpy(tmpbuff, buff, 1024);
	char* method = strtok(tmpbuff, " \t\n");	//equals POST or GET
	char* path = strtok(NULL, " \n"); // throw "\" away

	//case blank path
	if(strcmp(path, "/") == 0){	
		blank_get(client_fd, buff);
		return;
	}
	//case path is directory
	DIR* dir = opendir(path+1);
	if(dir != NULL){	
		generate_files(client_fd, dir);
	}
	//case path is file
	else if(access(path+1, F_OK) == 0){
		send_file(path+1, client_fd, buff);
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


int main(int argc, char *argv[]){
	int socket_fd, client_fd = -1;
	int success;
	unsigned sin_size;

	char buff[BUFFER_SIZE];

	//char* pwd = getenv("PWD");
	
	int port = 4000;
	assert(port > 1024 && port <= 65535);

	struct sockaddr_in my_addr, client_addr;


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

	while(true){
		sin_size = sizeof(struct sockaddr_in);
		client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &sin_size);
		printf("server: got connection from %s\n", inet_ntoa(client_addr.sin_addr));
		if(client_fd == -1){
			perror("Couldn't accept");
			continue;
		}
		while(true){
			memset(buff, '\0', BUFFER_SIZE);
			int read = recv(client_fd, buff, BUFFER_SIZE, 0);
			if(read == 0) continue;
			handle_request(buff, client_fd);
			break;
			//send(client_fd, "Hello, World!\n", 14, 0);
		}
	}









	return 0;
}