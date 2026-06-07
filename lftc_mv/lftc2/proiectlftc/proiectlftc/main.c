#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "ad.h"

int main() {
    char* inbuf = loadFile("testat.c");
    if (inbuf == NULL) {
        printf("Eroare: Nu s-a putut incarca fisierul testat.c\n");
        return 1;
    }

    Token* tokens = tokenize(inbuf);
    if (tokens == NULL) {
        printf("Eroare: Lexerul nu a produs niciun atom!\n");
        free(inbuf);
        return 1;
    }

    printf("Incepe analiza sintactica, de domeniu si de tipuri...\n");

    parse(tokens);

    printf("Analiza finalizata cu succes! Fișierul este corect.\n");

    free(inbuf);
    return 0;
}