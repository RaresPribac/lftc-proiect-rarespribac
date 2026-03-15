#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"

int main() {

    char* inbuf = loadFile("testlex.c");

    puts("Start:");
    Token* tokens = tokenize(inbuf);



    showTokens(tokens);


    free(inbuf);

    return 0;
}