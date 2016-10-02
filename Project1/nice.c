#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void do_nice(char** args, int isFlag, int priority){
	pid_t pid;
	pid = fork();
	if(pid<0){
		perror("Fork went wrong");
	}
	if(pid==0){//child
		if(isFlag == 1){
			int curPriority = getpriority(PRIO_PROCESS,getpid());
			nice(curPriority+priority);
		}else{
			nice(priority);
		}
		execvp(args[0],args);
		exit(0);
	}else{//parent
		wait(0);
	}

}

 


int zmain(){
	char* command[] = {"ls",NULL};
	do_nice(command,0,5);
}