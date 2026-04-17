#include "lexer.h"

#include <stdio.h>

typedef struct {
    char *name;
} TokenLookup;

TokenLookup lookup[] = {
    [TOKEN_EOF] = {"TOKEN_EOF"},
    [TOKEN_NUM] = {"TOKEN_NUM"},
    [TOKEN_STRING] = {"TOKEN_STRING"},

    [TOKEN_SEMICOLON] = {"TOKEN_SEMICOLON"},

    [TOKEN_STAR] = {"TOKEN_STAR"},
    [TOKEN_PLUS] = {"TOKEN_PLUS"},
    [TOKEN_MINUS] = {"TOKEN_MINUS"},
    [TOKEN_SLASH] = {"TOKEN_SLASH"},

    [TOKEN_PLUS_EQUALS] = {"TOKEN_PLUS_EQUALS"},

    [TOKEN_ERROR] = {"TOKEN_ERROR"},
    [TOKEN_UNKNOWN] ={"TOKEN_UNKNOWN"}
};

void printToken(const Token token) {
    printf("%04d | %s", token.line, lookup[token.type].name);

    switch (token.type) {
        case TOKEN_NUM:
            printf(", %lf", *(double*) token.data);
            break;
        case TOKEN_ERROR:
        case TOKEN_STRING:
            printf(", '%s'", (char*) token.data);
            break;
        case TOKEN_EOF:
        case TOKEN_UNKNOWN:
        default:
            break;

    }
    printf("\n");
}