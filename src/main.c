#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "lexer.h"
#include "parser.h"
#include "debug/lexer.h"

void parseFlags(int argc, char* argv[]) {
    //IDK do that parsing
}

int main(const int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Incorrect usage\n"
                              "Proper Usage: grtcmp <source file>\n"
                              "Use -h for help\n");
        exit(64);
    }

    parseFlags(argc, argv);

    ArenaAllocator *tokenData = ArenaNew();
    Lexer lexer; //last argument must be source file
    lexerInit(&lexer, argv[argc - 1], tokenData);
    ArrayList *tokens = scanAll(&lexer);
    free(lexer.source);

#ifdef DEBUG_PRINT_TOKENS

    for (u32 i = 0; i < tokens->size; i++) {
        const Token tok = ArrayListRead(tokens, i, Token); // get data
        printToken(tok);
    }

#endif

#ifdef DEBUG_TOKEN_COUNT
    char actualpath[PATH_MAX + 1];
    realpath(argv[argc - 1], actualpath);

    printf("%d Tokens in %s\n", tokens->size, actualpath);
#endif

    Parser parser;
    ParseResult ast = parseAll(&parser, tokens);

    if (parser.hadError) return 1;

    // build AST

    // optimise (?)

    // Compile to IR (?)

    // Compile to Assembly depending on flags

    return 0;
}