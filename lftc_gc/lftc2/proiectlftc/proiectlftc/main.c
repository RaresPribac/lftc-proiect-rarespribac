#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "ad.h"

int main() {
    // Incarcam fisierul destinat testarii generarii de cod (factorial recursiv si nerecursiv)
    char* inbuf = loadFile("testgc.c");
    if (inbuf == NULL) {
        printf("Eroare: Nu s-a putut incarca fisierul testgc.c\n");
        return 1;
    }

    Token* tokens = tokenize(inbuf);
    if (tokens == NULL) {
        printf("Eroare: Lexerul nu a produs niciun atom!\n");
        free(inbuf);
        return 1;
    }

    printf("Incepe compilarea si generarea de cod...\n");
    parse(tokens); // Lanseaza tot procesul

    free(inbuf);
    return 0;
}