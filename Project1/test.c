#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "vector.h"
#include "cd.c"
#include "echo.c"
#include "export.c"
#include "pwd.c"
#include "ulimit.c"
#include "type.c"
#include "kill.c"




/* Prints info about free shell */
int printFshInfo(){
	char* info = "This is free shell. Not everything is free in this life :)";
	printf("%s\n%s\n", info, "Our free shell has the following functions:");
	int i;
	for(i=0; i<sizeof(builtInFunctions)/sizeof(char*); i++){
		printf("%s%s\n", "     ", builtInFunctions[i]);
	}
	return 0;
}

//TODO status code = 0?
int callExit(int statusCode){
	exit(statusCode);
}

/* Parses incoming command line and saves all tokens in C string array 
	@param command - incoming command line */
void tokensFromCommand(char* command, vector* v){
	int i;
	int length =strlen(command);
	for(i=0; i<length; i++){
		if(*(command+i) == '"'){
			strcpy(command+i, command+i+1);
			length--;
			int j;
			for(j=i; j<length; j++){
				if(*(command+j) == '"'){
					*(command+j)='\0';
					i = j;
				}
			}
		} 
		if(*(command+i) == ' '){
			*(command+i) = '\0';
		}
	}
	
	VectorNew(v, sizeof(char*), NULL, 10);  //TODO freeFN

	for(i=0; i<length; i++){
		char* next = command+i;
		while(strlen(next) == 0){
			i++;
			next = command+i;
		}
		i+=strlen(next);
		VectorAppend(v, &next);
	}
}

/* Any operations needed to perform before application exits */
void performExit(vector* commands, char* command){
	VectorDispose(commands);
}

/* Executes one of the builtin functions */
int executeBuiltIn(vector commands, int size, char* firstKey, char* command){
	int exitCode = 0;
	if(strcmp(firstKey, "?") == 0){
		exitCode = printFshInfo();
		return exitCode;
	} else if(strcmp(firstKey, "cd") == 0){
		char* path;
		if(size > 1){
			path = *(char**)VectorNth(&commands, 1);
		} else {
			path = NULL;
		}
		exitCode = change_directory(path);
	} else if(strcmp(firstKey, "pwd") == 0){
		exitCode = print_working_directory();
	} else if(strcmp(firstKey, "exit") == 0){
		int statusCode = EXIT_SUCCESS;
		if(size > 1){
			statusCode = atoi(*(char**)VectorNth(&commands, 1));
		}
		performExit(&commands, command);
		callExit(statusCode);
	} else if(strcmp(firstKey, "ulimit") == 0){
		exitCode = get_limit(&commands);
	} else if(strcmp(firstKey, "nice") == 0){
	
		return 0;
	} else if(strcmp(firstKey, "kill") == 0){
		if(VectorLength(&commands) >= 3){
			exitCode = kill_it(*(char**)VectorNth(&commands, 1), *(char**)VectorNth(&commands, 2));
		}
		exitCode = -1;
	} else if(strcmp(firstKey, "type") == 0){
		return 0;
	} else if(strcmp(firstKey, "echo") == 0){
		do_echo(&commands);
	} else if(strcmp(firstKey, "export") == 0){
		char* arg;
		if(size > 1){
			arg = *(char**)VectorNth(&commands, 1);
		} else {
			arg = NULL;
		}
		export_variable(arg);
	}
	return exitCode;
}


int do_command(char* command){
	if(command[0] == '\0'){
		return 0;
	}

	if ( !strcmp (command, "1") ) {
		char* f1[]= {"ls",NULL};
		int res = execvp(f1[0], f1);
		if(res != 0){
			perror("couldn't call");
		}
		return 0;
	}

	if ( !strcmp (command, "2") ) {
		char* f1[]= {"sort", "-r", NULL};
		int res = execvp (f1[0], f1);
		if(res != 0){
			perror("couldn't call");
		}
		return 0;
	}

	vector commands;
	// while(true){



		tokensFromCommand(command, &commands);
		if(VectorLength(&commands) == 0) {
			// continue;
			VectorDispose(&commands);
		}
	
		char* firstKey = *(char**)VectorNth(&commands, 0);

		int size = VectorLength(&commands);

		if(isBuiltIn(firstKey)){
			executeBuiltIn(commands, size, firstKey, command);
		} else {
			pid_t pid = fork();
			if(pid == -1){
				perror("conflict durig fork!");
			} else if (pid == 0) {
				
 				int size = VectorLength(&commands) + 1;

 				char* cmd[size];

 				int i;
 				for(i=0; i<size-1; i++){
 					cmd[i] = *(char**)VectorNth(&commands,i);
 	
 				}

 				//for(i=0; i<size-1; i++){
 				//	printf("%s\n", cmd[i]);
 				//}
 				cmd[size-1] = NULL;

 				execvp(cmd[0], cmd);

			} else {
				
				wait(0);
			}
		}


		performExit(&commands, command);
	// }
	return 0;
}


