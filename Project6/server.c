
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdbool.h>
#include "dirent.h"

#define BACKLOG 128
#define BUFFER_SIZE 2000

void handle_request(char*, int);
void generate_files(int client_fd);
void blank_get(int);

/* Case when request came with "GET / " only */
void blank_get(int client_fd){
	if(access("index.html", F_OK) == 0){
		//send index.html
	} else {
		generate_files(client_fd);
	}
}
/* Generates list of files existing in the current directory
 * make html list of them and sends to client */
void generate_files(int client_fd){
	char* pwd = getenv("PWD");
	DIR *dir = opendir(pwd);
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
	if(strcmp(path, "/") == 0){
		blank_get(client_fd);
	}
	char* http_version = strtok(NULL, "\n");	//http version e.g. HTTP/1.1

	//printf("%s\n", method);
	//printf("%s\n", path);
	//printf("%s\n", http_version);
	//if(strncmp(http_version, "HTTP/1.1", 8) == 0)
	//if(strncmp(method, "GET", 3) == 0)
	//if(strncmp(method, "POST", 4) == 0)	

	strtok(NULL, " \t\n"); //throw "Host:" away
	char* host = strtok(NULL, " \t\n");

}


int main(int argc, char *argv[]){
	int socket_fd, client_fd = -1;
	int success;
	unsigned sin_size;

	char buff[BUFFER_SIZE];

	char* pwd = getenv("PWD");
	
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