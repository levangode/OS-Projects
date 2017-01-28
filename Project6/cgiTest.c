#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]){
	printf("HTTP/1.1 200 OK\r\n");
	printf("Content-Type: text/plain;charset=us-ascii\n");
	printf("Content-Length: %d\n\n" , strlen("<p>Hello</p>"));
	printf("<p>Hello</p>\n\n");
  return 0;
}