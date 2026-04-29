#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include "lexer.h"
#include "parser.h"
#include "debug/lexer.h"
#include "debug/parser.h"
#include "interpret/interpreter.h"

typedef enum {
    ARCH_NONE,
    ARCH_X86_64,
    ARCH_GRTCMP,
    ARCH_INTERPRET
} Architecture;

typedef struct {
    Architecture architecture;
    char *sourcefile;
} Flags;

Flags parseFlags(int argc, char* argv[]) {
    Flags flags =  {ARCH_NONE, nullptr};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) flags.architecture = ARCH_INTERPRET;
        else flags.sourcefile = argv[i];
    }
    return flags;
}

int main(const int argc, char* argv[]) {

    if (argc <= 2) {
        fprintf(stderr, "Incorrect usage\n"
                              "Proper Usage: grtcmp <source file>\n"
                              "Use -h for help\n");
        exit(64);
    }

    const Flags compileFlags = parseFlags(argc, argv);

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

    printf("%d Tokens in %s\n\n", tokens->size, actualpath);

#endif

    Parser parser;
    ParseResult ast = parseAll(&parser, tokens);

    if (parser.hadError) return 1;
#ifdef DEBUG_PRINT_AST

    for (u32 i = 0; i < ast.tree->size; i++) {
        printStmt(ArrayListRead(ast.tree, i, StmtNode*));
        printf("\n");
    }
    printf("\n");

#endif

    if (compileFlags.architecture == ARCH_INTERPRET) interpretProgram(ast.tree);



    // optimise (?)

    // Compile to IR (?)

    // Compile to Assembly depending on Flags

    return 0;
}