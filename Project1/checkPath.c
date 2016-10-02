#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

//struct dirent and DIR fount on wikipedia and course book and linux man
int checkDir(char* curDir,char* str){
	DIR*  directory;
	struct dirent *entry;
	directory = opendir(curDir);
	if(directory != NULL){
		while(1){
			entry = readdir(directory);
			if(entry != NULL){
				if (! (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 )){	
					continue;
				}			
				int res = strcmp(str,entry->d_name);
				if(res==0){
					closedir(directory);
					return 1;
				}
			}else{
				break;
			}
		}
	}else{
		perror("can't open directory");
	}
	closedir(directory);
	return 0;
}




int searchFile(char* str){
	char* paths = getenv("PATH");
	char* pos;
	int i =0;
	while(1){
		if(paths[i]==':'){
			paths[i] = '\0';
			char* curDir = paths;
			if(checkDir(curDir,str) == 1)return 1;
			paths =(char*)paths + strlen(paths)+1;
			i=-1;
		}else if(paths[i] =='\0'){
			break;
		}
		i++;
	}
	return 0;
} 


