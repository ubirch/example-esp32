#include "include/math_d.h"
#include <stdlib.h>
#include <string.h>

int plus(int a, int b){
    int c = 0;
    c= a+b;
    return c;
}

char * string_together(char *a, char *b){

    char *c = malloc(strlen(a)+strlen(b)*sizeof(char));
    c = strcat(a,b);
    return c;
}
