#include <stdlib.h>

int export_variable(char* arg){
	if(arg == NULL){
		return 0;
	} else {
		int res = putenv(arg);
		if(res != 0){
			perror("Couldn't set variable");
			return res;
		}
	}
	return 0;
}