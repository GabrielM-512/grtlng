#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "lexer.h"
#include "debug/lexer.h"

void parse_flags(int argc, char* argv[]) {
    //IDK do that parsing
}

int main(const int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Incorrect usage\n"
                              "Proper Usage: grtcmp <source file>\n"
                              "Use -h for help\n");
        exit(64);
    }

    parse_flags(argc, argv);

    ArenaAllocator *tokenData = ArenaNew();
    Lexer lexer; //last argument must be source file
    lexerInit(&lexer, argv[argc - 1], tokenData);
    ArrayList *tokens = scanAll(&lexer);
    free(lexer.source);

#ifdef DEBUG_PRINT_TOKENS

    for (int i = 0; i < tokens->size; i++) {
        const Token tok = *(Token*) ArrayListGet(tokens, i); // get data
        printToken(tok);
    }

#endif


    char actualpath[PATH_MAX + 1];
    realpath(argv[argc - 1], actualpath);

    printf("%d Tokens in %s\n", tokens->size, actualpath);

    // build AST

    // optimise (?)

    // Compile to IR (?)

    // Compile to Assembly depending on flags

    return 0;
}