#ifndef io_handler
#define io_handler

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "io.c"


void extract_file_name(char* str, char* dest){
	int begin = 0;
	
	while(str[begin] != '\0'){
//		printf("first while @ %d\n", begin);
		if( isspace( str[begin] ) ){
			begin++;
		} else {
			break;
		}
	}

	if (str[begin] == '\0'){
		perror("no filename specified\n");
	}

	int end = begin;

	//printf("%i\n", end);

	while(str[end] != '\0'){
		if( ! isspace(str[end]) ){
			end++;
		} else {
			break;
		}
	}

//	printf("3\n");

	//copy file name to destination;
	strncpy(dest, str+begin, end-begin);
	dest[end-begin] = '\0';

	//clear out file name from original string
	int i;
	for(i=begin; i<=end; i++){
		str[i] = ' ';
	}

//	printf("%i '%s'\n", end-begin, dest);
}

//temp function for testing
int test_main(){
	char a[100] = "   \n	z\n	 asd asdasda  ";
	char b[100];


	extract_file_name(a, b);

	return 0;
}


int parse_io(char* arg){
	int cur = 0;

	//int i;

	char fileName[1024];
	for(cur = 0; arg[cur]!='\0'; cur++){
		if( (arg[cur] == '>') && (arg[cur + 1] == '>') ){

			//clear out operator.
			arg[cur]	= ' ';
			arg[cur+1]	= ' ';

			extract_file_name(arg+cur, fileName);


			//printf("executing >> @ %s\n", fileName);
			redirect_to_file(fileName, 2);


			break;
		} else if(arg[cur] == '>'){
			
			arg[cur] = ' ';

			extract_file_name(arg+cur, fileName);

			printf("executing > @ %s\n", fileName);
			redirect_to_file(fileName, 1);

			break;

		} else if(arg[cur] == '<'){

			arg[cur] = ' ';

			extract_file_name(arg+cur, fileName);

			printf("executing < @ %s\n", fileName);

			redirect_from_file(fileName);
			break;
		}
	}


	return 0;
}

#endif