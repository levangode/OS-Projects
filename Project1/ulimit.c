#include <sys/time.h>
#include <sys/resource.h>
#include "vector.h"
#include <stdbool.h>
#include <ctype.h>
#include "helpers.c"

char* flags[] = {
	"-a",
	"-c",
	"-d",
	"-f",
	"-l",
	"-m",
	"-n",
	"-s",
	"-t",
	"-u",
	"-v",
	"-H",
	"-S"
};


/* Gets the limit */
int getlim(int resource, struct rlimit* rlim, char* name, int num, char* type){
	printf("%s\n", name);
	printf("%d\n", num);
	printf("%s\n", type);
	int res = getrlimit(resource, rlim);
	if(res == -1){
		perror("Couldn't get limit");
		return res;
	} 
	if(num != -2){
		if(type == NULL){
			rlim->rlim_cur = num;
			rlim->rlim_max = num;
		} else if(strcmp(type, "-S") == 0){
			rlim->rlim_cur = num;
		} else if(strcmp(type, "-H") == 0){
			rlim->rlim_max = num;
		}
		printf("%s\n", name);
		printf("%d\n", num);
		printf("%s\n", type);
		printf("%d\n", resource);
		res = setrlimit(resource, rlim);
		if(res == -1){
			perror("Couldn't set limits");
		}
		return res;
	}
	if(res == -1){
		perror("Couldn't get limit");
		return res;
	} else {
		printf("%s :", name);
		int l;
		if(type == NULL || strcmp(type, "-S") == 0){
			l = (int)rlim->rlim_cur;
		} else {
			l = (int)rlim->rlim_max;
		}
		if(l == -1){
			printf("%s\n", "unlimited");
		} else {
			printf("%d\n", l);	
		}
	}
	return 0;
}

/* Returns true if next argument is either "-S" or "-H" flag */
bool nextIsType(vector* args, int i){
	if(VectorLength(args) > i+1){
		char* z = *(char**)VectorNth(args, i+1);
		if(strcmp(z, "-S") == 0 || strcmp(z, "-H") == 0){
			return true;
		}
	}
	return false;
}

/* Returns true if next argument is number */
bool nextIsNum(vector* args, int i){
	if(VectorLength(args) > i+1){
		char* value = *(char**)VectorNth(args, i+1);
		if(isnumber(value)){
			return true;
		}
	}
	return false;
}


/* Prints all limits to stdout */
int printAllResources(vector* args, int i, struct rlimit* rlim){
	int num = -2;
	char* type = NULL;
	int res = 0;
	if(nextIsType(args, i)){
		i++;
		char* t = *(char**)VectorNth(args, i);
		type = t;
	}
	res = getlim(RLIMIT_CORE, rlim, "core file size", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_DATA, rlim, "data seg size", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_FSIZE, rlim, "file size", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_LOCKS, rlim, "max locked memory", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_RSS, rlim, "max resident set", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_NOFILE, rlim, "open files", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_STACK, rlim, "stack size", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_CPU, rlim, "cpu time", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_NPROC, rlim, "max user processes", num, type);
	if(res == -1) return res;
	res = getlim(RLIMIT_AS, rlim, "virtual memory", num, type);
	if(res == -1) return res;
	return res;
}

/* Determines if one command is finished */
bool isCommandEnd(vector* args, int i){
	if(VectorLength(args) < i+1) return true;
	char* arg = *(char**)VectorNth(args, i);
	int j;
	for(j=0; j<sizeof(flags)/sizeof(char*); j++){
		if(strcmp(arg, flags[j]) == 0) return true;
	}
	return false;
}

/* Wrapper function for getting limit. Does the parsing and argument passing job
 * Flags checked at http://ss64.com/bash/ulimit.html
 */
int get_limit(vector* args){
	struct rlimit rlim;
	int resource;
	char* name;
	int res = 0;

	int i;
	for(i=1; i<VectorLength(args); i++){	//Will call for each flag passed to ulimit
		char* flag = *(char**)VectorNth(args, i);
		if(strcmp(flag, "-a") == 0){
			res = printAllResources(args, i, &rlim);
			return res;
		} else if(strcmp(flag, "-c") == 0){
			name = "core file size";
			resource = RLIMIT_CORE;
		} else if(strcmp(flag, "-d") == 0){
			name = "data seg size";
			resource = RLIMIT_DATA;
		} else if(strcmp(flag, "-f") == 0){
			name = "file size";
			resource = RLIMIT_FSIZE;
		} else if(strcmp(flag, "-l") == 0){
			name = "max locked memory";
			resource = RLIMIT_LOCKS;
		} else if(strcmp(flag, "-m") == 0){
			name = "max resident set";
			resource = RLIMIT_RSS ;
		} else if(strcmp(flag, "-n") == 0){
			name = "open files";
			resource = RLIMIT_NOFILE;
		}  else if(strcmp(flag, "-s") == 0){
			name = "stack size";
			resource = RLIMIT_STACK;
		} else if(strcmp(flag, "-t") == 0){
			name = "cpu time";
			resource = RLIMIT_CPU;
		} else if(strcmp(flag, "-u") == 0){
			name ="max user processes";
			resource = RLIMIT_NPROC;
		} else if(strcmp(flag, "-v") == 0){
			name ="virtual memory";
			resource = RLIMIT_AS;
		} else {
			printf("%s\n", "Wrong command");
			return -1;
		}

		char* type = NULL;
		int num = -2;

		if(nextIsNum(args, i)){
			i++;
			num = atoi(*(char**)VectorNth(args, i));
			if(nextIsType(args, i)){
				i++;
				char* t = *(char**)VectorNth(args, i);
				type = t;
			}
		} else if(nextIsType(args, i)){
			i++;
			char* t = *(char**)VectorNth(args, i);
			type = t;
		}
		if(isCommandEnd(args, i+1)){
			res = getlim(resource, &rlim, name, num, type);
		}
	}
	return res;
}