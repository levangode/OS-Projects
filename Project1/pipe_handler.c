#ifndef pipe_handler
#define	pipe_handler

#include <string.h>
#include <stdio.h>

int parse_pipe(char* arg);

#include "twoWayPipe.c"
#include "io_handler.c"
#include "test.c"



int parse_pipe(char* arg){
	int cur = 0;

	// char z[] = "aasdasdasd";
	// printf("%s\n", z);
	//int i;
	for(cur = 0; arg[cur]!='\0'; cur++){
		//printf("1\n");
		if(arg[cur] == '|'){
			// printf("2\n");
			
			// printf("%s\n", z);
			arg[cur] = '\0';

			//printf("3\n");
			char* first = arg;
			char* second = (char*)arg + cur + 1;

			//printf("%s\n", first);
			//printf("%s\n", second);


			do_pipe(first, second);
			break;
		} 
	}

	parse_io(arg);

		do_command(arg);

	return 0;
}

int pumain(){
	char a[] = "1|2";

	
	parse_pipe(a);

	return 0;
}

#endif