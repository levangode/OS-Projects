#include <sys/types.h>
#include <signal.h>


/* Sends a signal to process passed by pid */
int kill_it(char* sig, char* pid){
	int p;
	if(sig[0] == '-'){
		sig++;
	}
	int s;
	if(isnumber(pid)){
		p=atoi(pid);
	} else {
		printf("%s\n", "Wrong format of ID");
		return -1;
	}
	if(isnumber(sig)){
		s=atoi(sig);
	} else {
		printf("%s\n", "Wrong format of ID");
		return -1;
	}
	printf("%d   %d\n",p,s);
	int res = kill(p, s);
	if(res != 0){
		perror("Sending signal failed");
	}
	return res;
}
