#include <unistd.h>
#include <string.h>
#include <stdbool.h>


/* Changes directory 
	* passed argument may be of type PATH or may be just the name of dir in current directory, may be NULL or ..
	* returns 0 on success
*/
int change_directory(char* dest){
	char* path = dest;
	bool was = false;
	if(dest == NULL){
		path = getenv("HOME");
		if(path == NULL){
			return -1;
		}
	} else if(*dest != '/'){
		char buff[1024];
		char* cur = getcwd(buff, 1024);
		int len = strlen(cur);
		path = malloc(len+strlen(path));
		was=true;
		strcpy(path, cur);
		strcat(path, "/");
		strcat(path, dest);
	}

	printf("%s\n", path);
	int rVal = chdir(path);
	if(was){
		free(path);
	}
	if(rVal == -1){
		perror("Cannot open directory");
	}
	return rVal;
}
