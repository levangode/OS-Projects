#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "vector.h"

#include "pipe_handler.c"

const int DEFAULT_FLAGS_COUNT = 5;

const char EXIT[5] = "exit\0";
const char EMPTY[1] = "\0";

void free_fn(void* elemAddr){
	free(elemAddr);
}

void print_fn(void* elemAddr, void* auxData){
    char** strAddr = (char**)elemAddr;

    printf("'%s'\n", *strAddr);
}

void sighandler_t(int arg){
    //printf("signal cancelled\n");

    // do nothing when
}

int execute_expression(vector* commands, char* operations){

    //VectorMap(commands, print_fn, NULL);

    //printf("now executing expression\n");
    int i;
    int res;
    for(i=0; i<VectorLength(commands); i++){
        //printf("1\n");
        char* command = *(char**)VectorNth(commands, i);
        //printf("2\n");

        //res = do_command(command);
        res = parse_pipe(command);


        if(i < sizeof(operations)){
            if(res != 0 && operations[i] == '&'){
                break;
            } else if(res == 0 && operations[i] == '|'){
                break;
            }
        }
    }
    return res;
}

// asdasdas | adasdasd || asdasd >> asdsad.txt | askjdgasd

int main(int argc, char *argv[])
{
    bool flagged = false;
    int res;
    char* flaggedarg;
    if(argc >= 3){
        char* firstArg = argv[1];
        if(strcmp(firstArg, "-c") == 0){
            flagged = true;
            flaggedarg = argv[2];
            //printf("%s\n", firstArg);
            //printf("%s\n", flaggedarg);
        }    
    }
    char* input, shell_prompt[1024];

    // bind tab key for auto completion
    rl_bind_key('\t', rl_complete);

    // disable ctrl + c signal
    signal(SIGINT, sighandler_t);

    // disable ctrl + z signal
    signal(SIGTSTP, sighandler_t);

    while(1) {
        snprintf(shell_prompt, sizeof(shell_prompt), "%s:%s $ ", getenv("USER"), getcwd(NULL, 1024));
        if(flagged){
            input = strdup(flaggedarg);
        } else {
		  input = readline(shell_prompt);
        }

        //printf("DEBUG info\n-------------\n");

        // if (strcmp(input, EXIT) == 0)
        //     break;


        add_history(input);
        
        //printf("%d", (int)strlen(input));

        //input[3] = '\0';
        		//VectorAppend(&commands, tmp);



        vector commands;

        VectorNew(&commands, sizeof(char*), free_fn, DEFAULT_FLAGS_COUNT);

        char operations[1000];
        int operationsCount = 0;

        // printf("%s\n", operations);


        int pointer = 0;
        int last = 0;

        while(1){
        	if(input[pointer] == '\0'){

                //printf("%d %d\n", last, pointer);

        		char* tmp = strdup( (char*) (input) + last );

                
        		VectorAppend(&commands, &tmp);

                
        		//printf("'%s'\n", tmp);

        		//free(tmp);

                operations[operationsCount] = '\0';

                operationsCount = 0;

                //printf("OPS_cnt -> %d\n", operationsCount );
                //printf("OPS_lst -> %s\n", operations );


        		break;
        	} else {
        		
        		if(input[pointer] == '&' && input[pointer+1] == '&'){
                    //printf("%d %d\n", last, pointer);

        			//printf("case 2\n");
        			input[pointer] = '\0';
                    input[pointer+1] = ' ';

        			char* tmp = strdup( (char*) (input) + last );

        			VectorAppend(&commands, &tmp);

        			//printf("'%s'\n", tmp);

        			//free(tmp);

        			last = pointer + 1;

        			operations[operationsCount++] = '&';

        		} else 

        		if(input[pointer] == '|' && input[pointer+1] == '|'){
                    //printf("%d %d\n", last, pointer);
        			
                    //printf("case 3\n");
        			input[pointer] = '\0';
                    input[pointer+1] = ' ';

        			char* tmp = strdup( (char*) (input) + last );

        			VectorAppend(&commands, &tmp);

        			//printf("'%s'\n", tmp);

        			//free(tmp);

        			last = pointer + 1;

        			
        			operations[operationsCount++] = '|';

        		}
                if(input[pointer] == ';'){
                    //printf("%d %d\n", last, pointer);
                    
                    //printf("case 3\n");
                    input[pointer] = '\0';

                    char* tmp = strdup( (char*) (input) + last );

                    VectorAppend(&commands, &tmp);

                    //printf("'%s'\n", tmp);

                    //free(tmp);

                    last = pointer + 1;

                    
                    operations[operationsCount++] = ';';

                }

        	}

        	pointer++;
        }

        //printf("%s\n", operations);

        //printf("%s\n", "now printing from vector>>");

        //VectorMap(&commands, print_fn, NULL);

        //printf("%s\n", "finished printing vector");

        //execute logical operations
        
        if(flagged){
            free(input);
            return execute_expression(&commands, operations);
        }

        // Free input.
        // VectorDispose(&commands);
        free(input);
        res = execute_expression(&commands, operations);
    }


    //printf("-----------\nend\n");
    return res;
}