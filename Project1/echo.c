#include "vector.h"
#include "string.h"
#include <stdlib.h>


int lastChildPid=0;


/* parses all commands and echoes as bash*/
int do_echo(vector* commands){
		int i;
		int res = 0;
		for(i=1; i<VectorLength(commands); i++){
			char* secondArg = *(char**)VectorNth(commands, i);
			if(secondArg[0] == '$'){
				if(strlen(secondArg) > 1){
					char* nextTok = secondArg+1;
					if(*nextTok == '?'){
						printf("%d ", lastChildPid);
						if(strlen(nextTok) > 1){
							printf("%s ", nextTok+1);
						}
					} else {
						char* varName = nextTok;
						char* varValue = getenv(varName);
						if(varValue != NULL){
							printf("%s", varValue);
						}
					}
				}
			} else {
				if(i == VectorLength(commands) -1){
					printf("%s", *(char**)	VectorNth(commands, i));
				} else {
					printf("%s ", *(char**)	VectorNth(commands, i));
				}
			}
		}
		printf("\n");
		return res;
}