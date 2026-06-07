#pragma once

enum {
    ID,
    // keywords
    TYPE_INT,
    // delimiters
    COMMA, SEMICOLON, END,
    // operators
    ASSIGN
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
