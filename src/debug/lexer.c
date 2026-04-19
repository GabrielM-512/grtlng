#include "lexer.h"
#include "../util/ArenaAlloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
} TokenLookup;

ArenaAlloc *text = nullptr;

TokenLookup lookup[TOKEN_UNKNOWN + 1];

/**
 * @brief Automatically finds the literals for the token names from 'src/lexer.h' and stores
 * them into the lookup table as strings.
 *
 */
void populate_table() {
    size_t length = 0;
    char* source;
    {
        const char* filename = "/home/gabriel/CLionProjects/language/src/lexer.h";
        FILE* file = fopen(filename, "r");
        fseek(file, 0L, SEEK_END);
        length = ftell(file) + 1;
        source = malloc(length);

        // reset to start of file
        rewind(file);

        //read the file
        fread(source, sizeof(char), length, file);
        fclose(file);

        source[length] = '\0';
    }



    // look for the enum
    int start;
    for (int i = 0; ; i++) {
        if (i + 9 > length) {
            fprintf(stderr, "[DEBUG ERROR] %s: Couldnt locate enum start", __FILE__);
            exit(1);
        }

        const char *target = "TOKEN_EOF";
        if (memcmp(target, &source[i], 9) == 0) {
            start = i;
            break;
        }
    }

    text = ArenaAllocNew();

    for (int i = 0; i <= TOKEN_UNKNOWN; i++) {
        int tokenlength = 0;

        // search for token end
        char c = source[start];
        while (c != ',') {
            tokenlength++;
            c = source[start + tokenlength];
        }
        tokenlength++;

        // store into the lookup table

        char *textdata = ArenaAllocAlloc(text, tokenlength);

        memcpy(textdata, &source[start], tokenlength);
        textdata[tokenlength - 1] = '\0';

        start += tokenlength;

        lookup[i].name = textdata;


        // skip whitespace
        c = source[start];
        while (c != 'T') {
            start++;
            c = source[start];
        }

    }

    free(source);
}


void printToken(const Token token) {
    if (text == nullptr) populate_table();

    printf("%04d | %s", token.line, lookup[token.type].name);

    switch (token.type) {
        case TOKEN_NUM:
            printf(", %lf", *(double*) token.data);
            break;
        case TOKEN_ERROR:
        case TOKEN_STRING:
            printf(", '%s'", (char*) token.data);
            break;
        default:
            break;

    }
    printf("\n");
}