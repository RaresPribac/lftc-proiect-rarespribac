#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lexer.h"
#include "utils.h"

Token* tokens = NULL;
Token* lastTk = NULL;
int line=1;

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
        case '(': addTk(LPAR); pch++; break;
        case ')': addTk(RPAR); pch++; break;
        case '[': addTk(LBRACKET); pch++; break;
        case ']': addTk(RBRACKET); pch++; break;
        case '{': addTk(LACC); pch++; break;
        case '}': addTk(RACC); pch++; break;
        case '+': addTk(ADD); pch++; break;
        case '-': addTk(SUB); pch++; break;
        case '*': addTk(MUL); pch++; break;
        case '/':
            if (pch[1] == '/') {
                pch += 2;
                while (*pch != '\n' && *pch != '\r' && *pch != '\0') pch++;
            }
            else {
                addTk(DIV);
                pch++;
            }
            break;
        case '&':
            if (pch[1] == '&') { addTk(AND); pch += 2; }
            else err("Lipseste al doilea & la linia %d", line);
            break;
        case '|':
            if (pch[1] == '|') { addTk(OR); pch += 2; }
            else err("Lipseste al doilea | la linia %d", line);
            break;
        case '!':
            if (pch[1] == '=') { addTk(NOTEQ); pch += 2; }
            else { addTk(NOT); pch++; }
            break;
        case '=':
            if (pch[1] == '=') { addTk(EQUAL); pch += 2; }
            else { addTk(ASSIGN); pch++; }
            break;
        case '<':
            if (pch[1] == '=') { addTk(LESSEQ); pch += 2; }
            else { addTk(LESS); pch++; }
            break;
        case '>':
            if (pch[1] == '=') { addTk(GREATEREQ); pch += 2; }
            else { addTk(GREATER); pch++; }
            break;
        case '"':
            pch++; 
            start = pch;
            char buffer[1024];
            int pos = 0;
            while (*pch && *pch != '"') {
                if (*pch == '\\') {
                    pch++; 
                    switch (*pch) {
                    case 'n': buffer[pos++] = '\n'; break;
                    case 't': buffer[pos++] = '\t'; break;
                    case 'r': buffer[pos++] = '\r'; break;
                    case '\\': buffer[pos++] = '\\'; break;
                    case '"': buffer[pos++] = '"'; break;
                    default: buffer[pos++] = *pch; break;
                    }
                }
                else {
                    buffer[pos++] = *pch;
                }
                pch++;
            }

            if (*pch == '"') {
                buffer[pos] = '\0';
                tk = addTk(STRING);
                tk->text = (char*)safeAlloc(pos + 1);
                strcpy(tk->text, buffer);
                pch++;
            }
            else {
                err("String neterminat la linia %d", line);
            }
            break;
        case '\'':
            pch++;
            start = pch;
            if (*pch == '\\') pch += 2; else pch++;
            if (*pch == '\'') {
                tk = addTk(CHAR);
                char* tmp = extract(start, pch);
                if (tmp[0] == '\\') {
                    if (tmp[1] == 'n') tk->c = '\n';
                    else if (tmp[1] == 't') tk->c = '\t';
                    else if (tmp[1] == 'r') tk->c = '\r';
                    else tk->c = tmp[1];
                }
                else tk->c = tmp[0];
                free(tmp); pch++;
            }
            else err("Caracter neterminat la linia %d", line);
            break;
        default:
            if (isalpha(*pch) || *pch == '_') {
                start = pch++;
                while (isalnum(*pch) || *pch == '_') pch++;
                char* text = extract(start, pch);
                if (strcmp(text, "char") == 0) addTk(TYPE_CHAR);
                else if (strcmp(text, "double") == 0) addTk(TYPE_DOUBLE);
                else if (strcmp(text, "else") == 0) addTk(ELSE);
                else if (strcmp(text, "if") == 0) addTk(IF);
                else if (strcmp(text, "int") == 0) addTk(TYPE_INT);
                else if (strcmp(text, "return") == 0) addTk(RETURN);
                else if (strcmp(text, "struct") == 0) addTk(STRUCT);
                else if (strcmp(text, "void") == 0) addTk(VOID);
                else if (strcmp(text, "while") == 0) addTk(WHILE);
                else {
                    tk = addTk(ID);
                    tk->text = text;
                }
            }
            else if (isdigit(*pch) || (*pch == '.' && isdigit(pch[1]))) {
                start = pch;
                bool isDouble = false;
                if (isdigit(*pch)) {
                    while (isdigit(*pch)) pch++;
                }
                if (*pch == '.') {
                    isDouble = true;
                    pch++;
                    if (!isdigit(*pch)){
                        err ("lipsa parte zecimala");}
                    while (isdigit(*pch)) pch++;
                }
                if (*pch == 'e' || *pch == 'E') {
                    isDouble = true;
                    pch++;
                    if (*pch == '+' || *pch == '-') pch++;
                    if (isdigit(*pch)) {
                        while (isdigit(*pch)) pch++;
                    }
                    else err("Exponent invalid la linia %d", line);
                }
                char* text = extract(start, pch);
                if (isDouble) {
                    tk = addTk(DOUBLE);
                    tk->d = strtod(text, NULL);
                }
                else {
                    tk = addTk(INT);
                    tk->i = atoi(text);
                }
                free(text);
            }
            else if (*pch == '.') {
                addTk(DOT);
                pch++;
            }
            else {
                err("Caracter invalid: %c (%d) la linia %d", *pch, *pch, line);
            }
        }
    }
}

void showTokens(const Token* tokens) {
    for (const Token* tk = tokens; tk; tk = tk->next) {
        printf("%d ", tk->line);
        switch (tk->code) {
        case ID: printf("ID:%s\n", tk->text); break;
        case TYPE_CHAR: printf("TYPE_CHAR\n"); break;
        case TYPE_DOUBLE: printf("TYPE_DOUBLE\n"); break;
        case TYPE_INT: printf("TYPE_INT\n"); break;
        case IF: printf("IF\n"); break;
        case ELSE: printf("ELSE\n"); break;
        case WHILE: printf("WHILE\n"); break;
        case RETURN: printf("RETURN\n"); break;
        case STRUCT: printf("STRUCT\n"); break;
        case VOID: printf("VOID\n"); break;
        case INT: printf("INT:%d\n", tk->i); break;
        case DOUBLE:
            printf("DOUBLE:%.1f\n", tk->d);
            break;
        case CHAR: printf("CHAR:%c\n", tk->c); break;
        case STRING:
            printf("STRING:%s\n", tk->text);
            break;
        case COMMA: printf("COMMA\n"); break;
        case SEMICOLON: printf("SEMICOLON\n"); break;
        case LPAR: printf("LPAR\n"); break;
        case RPAR: printf("RPAR\n"); break;
        case LACC: printf("LACC\n"); break;
        case RACC: printf("RACC\n"); break;
        case LBRACKET: printf("LBRACKET\n"); break;
        case RBRACKET: printf("RBRACKET\n"); break;
        case ASSIGN: printf("ASSIGN\n"); break;
        case EQUAL: printf("EQUAL\n"); break;
        case NOTEQ: printf("NOTEQ\n"); break;
        case ADD: printf("ADD\n"); break;
        case SUB: printf("SUB\n"); break;
        case MUL: printf("MUL\n"); break;
        case DIV: printf("DIV\n"); break;
        case DOT: printf("DOT\n"); break;
        case AND: printf("AND\n"); break;
        case OR: printf("OR\n"); break;
        case NOT: printf("NOT\n"); break;
        case LESS: printf("LESS\n"); break;
        case LESSEQ: printf("LESSEQ\n"); break;
        case GREATER: printf("GREATER\n"); break;
        case GREATEREQ: printf("GREATEREQ\n"); break;
        case END: printf("END\n"); break;
        default: printf("TOKEN_CODE:%d\n", tk->code);
        }
    }
}