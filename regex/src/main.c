#include <stdio.h>
#include <string.h>


#include "regex.h"


int main() {

    char regex[257] = {0};
    printf("regex: ");
    fgets(regex, sizeof(regex), stdin);

    //strip newline
    int l = strlen(regex);    
    regex[l - 1] = 0;

    Regex r = CreateRegex(regex);

    while (1) {
        char s[257] = {0};
        printf("string: ");
        fgets(s, sizeof(s), stdin);

        //strip newline
        int l = strlen(s);    
        s[l - 1] = 0;

        printf("%d\n", Match(r, s));

        if (!s[0]) {
            break;
        }
    }

    FreeRegex(&r);
    r = (Regex){0};

    return 0;
}
