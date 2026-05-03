#include "lexer.h"
#include "debugInfos.h"

#include <stdio.h>

/**
 * @brief Automatically finds the literals for the token names from 'src/lexer.h' and stores
 * them into the lookup table as strings.
 *
 */

void printTokenError(const Token token) {
    if (hasFailed()) return;

    // print token name left aligned with a width of 20 characters
    fprintf(stderr,"%04d | %-20.20s", token.line, getTokenName(token.type));

    switch (token.type) {
        case TOKEN_NUM:
            fprintf(stderr,"| %lf", *(double*) token.data);
            break;
        case TOKEN_ERROR:
        case TOKEN_STRING:
            fprintf(stderr,"| '%s'", (char*) token.data);
            break;
        case TOKEN_IDENTIFIER:
            fprintf(stderr,"| %s", (char*) token.data);
            break;
        default:
            break;

    }
    fprintf(stderr, "\n");
}

void printToken(const Token token) {
    if (hasFailed()) return;

    // print token name left aligned with a width of 20 characters
    printf("%04d | %-20.20s", token.line, getTokenName(token.type));

    switch (token.type) {
        case TOKEN_NUM:
            printf("| %lf", *(double*) token.data);
            break;
        case TOKEN_ERROR:
        case TOKEN_STRING:
            printf("| '%s'", (char*) token.data);
            break;
        case TOKEN_IDENTIFIER:
            printf("| %s", (char*) token.data);
            break;
        default:
            break;

    }
    printf("\n");
}