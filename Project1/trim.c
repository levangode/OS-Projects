#include <ctype.h>
#include <string.h>

void trim(char** str1){
    char* str = *str1;
    int prefix = 0;
    int suffix = 0;
    int lenght = strlen(str);
    int i;
    for(i=0; i<strlen(str); i++){
        if(isspace(str[i])){
            prefix++;
        } else {
            break;
        }
    }
    for(i=strlen(str)-1; i>=0; i--){
        if(isspace(str[i])){
            suffix++;
        } else {
            break;
        }
    }

    char* tmp = malloc(strlen(str) - prefix - suffix + 1);

    strncpy(tmp, str+prefix, lenght - prefix - suffix + 1);

    free(str);

    *str1 = tmp;

    printf("%d, %d, %d\n", prefix, suffix, (int)strlen(tmp));
}
