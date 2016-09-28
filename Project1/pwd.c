#include <unistd.h>

/* Gets working directory. No special logics */
int print_working_directory(){
	int res = 0;
	char buff[1024];
	char* cwd = getcwd(buff, 1024);
	if(cwd == NULL){
		perror("Couldn't get directory");
		return -1;
	} else {
		printf("%s\n", cwd);
	}
	return res;
}