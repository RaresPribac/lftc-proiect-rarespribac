#pragma once

enum {
    ID,
    // keywords
    TYPE_INT, TYPE_CHAR, TYPE_DOUBLE, IF, ELSE, WHILE, RETURN, STRUCT, VOID,
    // delimiters
    COMMA, SEMICOLON, LPAR, RPAR, LACC, RACC, LBRACKET, RBRACKET, END,
    // operators
    ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
    ADD, SUB, MUL, DIV, AND, OR, NOT, DOT,
    // constante
    INT, DOUBLE, CHAR, STRING
};

typedef struct Token {
    int code;
    int line;
    union {
        char* text;
        int i;
        char c;
        double d;
    };
    struct Token* next;
} Token;

Token* tokenize(const char* pch);
void showTokens(const Token* tokens);