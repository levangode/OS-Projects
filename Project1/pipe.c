#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "vector.h"


void do_pipe(char*** args,int argc){
    for(int i=0; i<argc-1; i++){
        int pd[2];
        pipe(pd);
        pid_t pid = fork();
        if(pid == -1){
            perror("Something wrong with fork");
            exit(0);
        }else if (pid == 0) {
            close(1);
            dup(pd[1]); 
            execvp(args[i][0],args[i]);
            exit(0);
        }else{
            close(0);
            dup(pd[0]);
            close(pd[1]);
            wait(0);
        }
    }
    execvp(args[argc-1][0],args[argc-1]);
    exit(0);
}



