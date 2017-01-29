#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]){

	printf("HTTP/1.1 200 OK\r\n");
	printf("Content-Type: text/html\r\n");
	printf("Content-Length: %d\r\n\r\n" , strlen("<p>Hello</p>\r\n"));
	printf("<p>Hello</p>\r\n");
  return 0;
} 