#include "parser.h"

#include <stdio.h>

/*typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_SUM,
    PREC_PRODUCT
} ExprPrecedence;*/

typedef TreeNode*(*ParseFn)(Parser*);

static void advance(Parser* parser);

/*typedef struct {
    ParseFn prefix;
    ParseFn infix;
    ExprPrecedence precedence;
} ParseRule;

ParseRule rules [] = {
};*/

TreeNode *expression(Parser* parser);

// externally callable function(s)

ParseResult parseAll(Parser *parser, ArrayList *tokens) {
    parser->Tokens = tokens;
    parser->token = 0;

    parser->program.tree = ArrayListNew(sizeof(TreeNode*));
    parser->program.data = ArenaNew();

    parser->hadError = false;
    parser->panicMode = false;

    advance(parser);

    while (parser->current.type != TOKEN_EOF) {
        expression(parser);
    }

    return parser->program;
}

// error handling

void parseErrorAt(Parser *parser, const Token* token, const char* message) {
    if (parser->panicMode) return;

    parser->panicMode = true;
    parser->hadError = true;

    fprintf(stderr, "Encountered error on line %d: %s\n", token->line, (char*) message);

    // synchronise now

}

void parseError(Parser *parser, const char* message) {
    parseErrorAt(parser, &parser->current, message);
}

// utils

static void advance(Parser *parser) {
    while (true) {
        parser->current = ArrayListRead(parser->Tokens, parser->token++, Token);
        if (parser->current.type != TOKEN_ERROR) break;

        parseError(parser, parser->current.data);
    }
}

void consume(Parser *parser, TokenType type, const char *message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    parseError(parser, message);
}

// internal functions

TreeNode *expression(Parser* parser) {
    advance(parser);
    return ArenaAlloc(parser->program.data, sizeof(TreeNode));
}