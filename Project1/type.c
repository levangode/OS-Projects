#include "vector.h"


char* builtInFunctions[] = {
	"?",
	"cd",
	"pwd",
	"exit",
	"ulimit",
	"nice",
	"kill",
	"type",
	"echo",
	"export"
};

/* Returs true is passed string argument is a builtIn function */
bool isBuiltIn(char* incoming){
	int i;
	for(i=0; i<sizeof(builtInFunctions)/sizeof(char*); i++){
		if(strcmp(incoming, builtInFunctions[i]) == 0){
			return true;
		}
	}
	return false;
}

bool isOtherProg(char* incoming){
	return false;
}

int get_type(vector* args){
	int i;
	for(i=1; i<VectorLength(args); i++){
		char* nextCommand = *(char**)VectorNth(args, i);
		if(isBuiltIn(nextCommand)){
			printf("%s %s\n", nextCommand, "is BuiltIn");
		}
	}
	return 0;
}