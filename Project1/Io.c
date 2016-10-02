#ifndef _io_
#define _io_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//1>  2>>
void redirect_from_file(char* fileName){
	int in = open(fileName,O_RDONLY);
	dup2(in,STDIN_FILENO);
	close(in);
}

void redirect_to_file(char* fileName, int type){
	int in;
	if(type == 1){
		in = open(fileName,O_WRONLY);
	}else if(type==2){
		in = open(fileName,O_APPEND|O_WRONLY);
	}
	dup2(in,1);
	close(in);
}



int prmain(){
	redirect_from_file("bla.txt");
	execlp("sort","sort",NULL);
	return 0;	
}

#endif