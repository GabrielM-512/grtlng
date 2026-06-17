#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/ArenaAllocator.h"
#include "util/ArrayList.h"

#include "lexer.h"
#include "parser/parser.h"
#include "debug/lexer.h"
#include "debug/parser.h"
#include "interpret/interpreter.h"
#include "error.h"

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
        else {
            flags.sourcefile = argv[i];
            if (i + 1 != argc) {
                fprintf(stderr, "Incorrect usage:\n"
                                      "  Source File must be last argument");
                exit(1);
            }
            break;
        }
    }
    return flags;
}

int main(const int argc, char* argv[]) {

    if (argc <= 2) {
        fprintf(stderr, "Incorrect usage\n"
                              "  Proper Usage: grtcmp <source file>\n"
                              "  Use -h for help\n");
        exit(64);
    }

    const Flags compileFlags = parseFlags(argc, argv);

    errorSetup();

    ArenaAllocator *tokenData = ArenaNew();
    Lexer lexer;
    lexerInit(&lexer, compileFlags.sourcefile, tokenData);
    ArrayList *tokens = scanAll(&lexer);

#ifdef DEBUG_PRINT_TOKENS

    for (u32 i = 0; i < tokens->length; i++) {
        const Token tok = ArrayListRead(tokens, i, Token); // get data
        printToken(tok);
    }

#endif

#ifdef DEBUG_TOKEN_COUNT

    #include <linux/limits.h>

    char actualPath[PATH_MAX + 1];
    realpath(compileFlags.sourcefile, actualPath);

    printf("%d Tokens in %s\n\n", tokens->length, actualPath);

#endif

    Parser parser;
    const ParseResult ast = parseAll(&parser, tokens, lexer.source);

    if (parser.hadError) {
        printErrors();
        return 1;
    }
#ifdef DEBUG_PRINT_AST

    printProgram(ast);
    printf("\n");

#endif

    if (compileFlags.architecture == ARCH_INTERPRET) interpretProgram(ast);



    // optimise (?)

    // Compile to IR (?)

    // Compile to Assembly depending on Flags

    return 0;
}