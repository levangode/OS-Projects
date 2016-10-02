#include <unistd.h>
#include <string.h>
#include <stdbool.h>


/* Changes directory 
	* passed argument may be of type PATH or may be just the name of dir in current directory, may be NULL or ..
	* returns 0 on success
*/
int change_directory(char* dest){
	char* path = dest;
	int rVal = chdir(path);
	if(rVal == -1){
		perror("Cannot open directory");
	}
	return rVal;
}
