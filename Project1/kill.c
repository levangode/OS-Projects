#include <sys/types.h>
#include <signal.h>
#include "helpers.c"

/* Sends a signal to process passed by pid */
int kill_it(char* pid, char* sig){
	int p;
	int s;
	if(isnumber(pid)){
		p=atoi(pid);
	}  else if(isnumber(pid)){
		s=atoi(sig);
	} else {
		printf("%s\n", "Wrong format of ID");
		return -1;
	}
	int res = kill(p, s);
	if(res != 0){
		perror("Sending signal failed");
		return res;
	}
}
