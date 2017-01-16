
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

#define BACKLOG 128
#define BUFFER_SIZE 1024
#define POST_GET_BUFFER 5
void parese(char*);

void parse(char* buff){
	char tmpbuff[1024];
	memcpy(tmpbuff, buff, 1024);

	//char method[POST_GET_BUFFER];
	char* method = strtok(tmpbuff, " \t\n");	//equals POST or GET
	strtok(NULL, " \n"); // throw "\" away
	char* http_version = strtok(NULL, "\n");	//http version e.g. HTTP/1.1

	//if(strncmp(http_version, "HTTP/1.1", 8) == 0)
	//if(strncmp(method, "GET", 3) == 0)
	//if(strncmp(method, "POST", 4) == 0)	

	strtok(NULL, " \t\n"); //throw "Host:" away
	char* host = strtok(NULL, " \t\n");
	printf("%s\n", host);

}


int main(int argc, char *argv[]){
	int socket_fd, client_fd = -1;
	int success;
	unsigned sin_size;
	char buff[BUFFER_SIZE];

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
			int read = recv(client_fd, buff, BUFFER_SIZE, 0);
			if(read == 0) continue;
			parse(buff);
			send(client_fd, "Hello, World!\n", 14, 0);
		}
	}









	return 0;
}