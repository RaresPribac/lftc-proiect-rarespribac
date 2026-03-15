#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token* tokens = NULL;
Token* lastTk = NULL;
int line = 1;

Token* addTk(int code) {
    Token* tk = (Token*)safeAlloc(sizeof(Token));
    tk->code = code;
    tk->line = line;
    tk->next = NULL;
    if (lastTk) {
        lastTk->next = tk;
    }
    else {
        tokens = tk;
    }
    lastTk = tk;
    return tk;
}

char* extract(const char* begin, const char* end) {
    size_t n = end - begin;
    char* s = (char*)safeAlloc(n + 1);
    memcpy(s, begin, n);
    s[n] = '\0';
    return s;
}

Token* tokenize(const char* pch) {
    const char* start;
    Token* tk;
    for (;;) {
        switch (*pch) {
        case ' ':
        case '\t':
            pch++;
            break;
        case '\r':
            if (pch[1] == '\n') pch++;
        case '\n':
            line++;
            pch++;
            break;
        case '\0':
            addTk(END);
            return tokens;
        case ',': addTk(COMMA); pch++; break;
        case ';': addTk(SEMICOLON); pch++; break;
        case '=': addTk(ASSIGN); pch++; break;
        default:
            if (isalpha(*pch) || *pch == '_') {
                for (start = pch++; isalnum(*pch) || *pch == '_'; pch++);
                char* text = extract(start, pch);
                tk = addTk(ID);
                tk->text = text;
            }
            else {
                err("Caracter invalid: %c (%d) la linia %d", *pch, *pch, line);
            }
        }
    }
}

void showTokens(const Token* tokens) {
    for (const Token* tk = tokens; tk; tk = tk->next) {
        printf("%d\t", tk->line);
        switch (tk->code) {
        case ID: printf("ID:%s\n", tk->text); break;
        case TYPE_INT: printf("TYPE_INT\n"); break;
        case COMMA: printf("COMMA\n"); break;
        case SEMICOLON: printf("SEMICOLON\n"); break;
        case ASSIGN: printf("ASSIGN\n"); break;
        case END: printf("END\n"); break;
        default: printf("UNKNOWN:%d\n", tk->code);
        }
    }
}