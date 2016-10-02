#ifndef	_pipe_
#define	_pipe_


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "pipe_handler.c"

void do_pipe(char* a, char* b);

void do_pipe(char* f1, char* f2){
	int p[2];
	pipe(p);
	pid_t pid = fork();
	if(pid==1){
		perror("Something went wrong with fork");
	}else if(pid == 0){//child
		close(p[1]);
		dup2(p[0],0);
		//execvp(f2[0],f2);
		parse_pipe(f2);
		exit(0);
	}else{//parent
		close(p[0]);
		dup2(p[1],1);
		//execvp(f1[0],f1);
		parse_pipe(f1);
		wait(0);
	}
}



int pmain(){
	//char* f1[]= {"ls",NULL};
	//char* f2[] ={"head","-2",NULL};
	//do_pipe(f1,f2);
	return 0;
}

#endif	