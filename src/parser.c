#include "parser.h"

#include <stdio.h>

#include "debug/lexer.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_SUM,
    PREC_PRODUCT
} ExprPrecedence;

typedef TreeNode*(*PrefixFn)(Parser*);
typedef TreeNode*(*InfixFn)(Parser*, TreeNode*);

typedef struct {
    PrefixFn prefix;
    InfixFn infix;
    ExprPrecedence precedence;
} ParseRule;

static void advance(Parser* parser);
void consume(Parser *parser, TokenType type, const char *message);

ParseRule getRule(TokenType token);

TreeNode *parseExpr(Parser* parser, ExprPrecedence precedence);

// externally callable function(s)

ParseResult parseAll(Parser *parser, ArrayList *tokens) {
    parser->Tokens = tokens;
    parser->token = 0;

    parser->program.tree = ArrayListNew(sizeof(TreeNode*));
    parser->program.data = ArenaNew();

    parser->hadError = false;
    parser->panicMode = false;

    advance(parser);


    TreeNode *expr = parseExpr(parser, PREC_ASSIGNMENT);
    ArrayListAdd(parser->program.tree, &expr);

    consume(parser, TOKEN_EOF, "Expected end of file");

    return parser->program;
}

// error handling

void parseErrorAt(Parser *parser, const Token* token, const char* message) {
    if (parser->panicMode) {
        return;
    }

    parser->panicMode = true;
    parser->hadError = true;

    fprintf(stderr, "Encountered error on line %d: %s\n", token->line, (char*) message);

    // synchronise here

}

void parseErrorAtCurrent(Parser *parser, const char* message) {
    parseErrorAt(parser, &parser->current, message);
}

void parseError(Parser *parser, const char* message) {
    parseErrorAt(parser, &parser->previous, message);
}

// utils

static bool isAtEnd(const Parser *parser) {
    return parser->Tokens->size <= parser->token;
}

static void advance(Parser *parser) {
    parser->previous = parser->current;
    while (true) {
        if (parser->token >= parser->Tokens->size) {
            parser->current = ArrayListRead(parser->Tokens, parser->Tokens->size - 1, Token);
        } else {
            parser->current = ArrayListRead(parser->Tokens, parser->token++, Token);
        }

        if (parser->current.type != TOKEN_ERROR) break;

        parseErrorAtCurrent(parser, parser->current.data);
    }
}

void consume(Parser *parser, TokenType type, const char *message) {
    if (parser->current.type == type) {
        if (!isAtEnd(parser)) advance(parser);
        return;
    }
    parseError(parser, message);
}

// internal functions

TreeNode *exprBinary(Parser *parser, TreeNode *left) {
    BinaryExprNode *node = ArenaAlloc(parser->program.data, sizeof(BinaryExprNode));

    node->header.type = NODE_BINARY_EXPR;

    node->operator = parser->previous.type;
    node->left = left;

    node->right = parseExpr(parser, getRule(parser->previous.type).precedence);

    return (TreeNode*) node;

}

TreeNode *exprUnary(Parser *parser) {
    UnaryExprNode *node = ArenaAlloc(parser->program.data, sizeof(UnaryExprNode));

    node->header.type = NODE_UNARY_EXPR;
    node->operator = parser->previous.type;

    node->right = parseExpr(parser, getRule(parser->previous.type).precedence);

    return (TreeNode*) node;
}

static TreeNode *number(Parser *parser) {
    NumberNode *node = ArenaAlloc(parser->program.data, sizeof(NumberNode));

    node->header.type = NODE_NUMBER;
    node->value = * (double*) parser->previous.data;

    return (TreeNode*) node;
}

TreeNode *grouping(Parser *parser) {
    TreeNode *node = parseExpr(parser, getRule(parser->previous.type).precedence);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')'");
    return node;
}


TreeNode *parseExpr(Parser* parser, ExprPrecedence precedence) {
    advance(parser);

    const PrefixFn prefixRule = getRule(parser->previous.type).prefix;

    if (prefixRule == nullptr) {

        parseError(parser, "Unexpected Token");
        printTokenError(parser->previous);

        TreeNode *node = ArenaAlloc(parser->program.data, sizeof(TreeNode));
        node->type = NODE_ERROR;

        return node;
    }

    TreeNode *left = prefixRule(parser);

    while (precedence < getRule(parser->current.type).precedence) {
        advance(parser);
        const InfixFn infixRule = getRule(parser->previous.type).infix;
        left = infixRule(parser, left);
    }

    return left;

}

ParseRule rules [] = {
    [TOKEN_EOF]             = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_ERROR]           = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_NUM]             = {number,      nullptr,    PREC_NONE   },
    [TOKEN_STRING]          = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_SEMICOLON]       = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_LEFT_PAREN]      = {grouping,    nullptr,    PREC_NONE   },
    [TOKEN_RIGHT_PAREN]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_LEFT_BRACE]      = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_RIGHT_BRACE]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_LEFT_BRACKET]    = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_RIGHT_BRACKET]   = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_PLUS]            = {nullptr,     exprBinary, PREC_SUM    },
    [TOKEN_MINUS]           = {exprUnary,   exprBinary, PREC_SUM    },
    [TOKEN_STAR]            = {nullptr,     exprBinary, PREC_PRODUCT},
    [TOKEN_SLASH]           = {nullptr,     exprBinary, PREC_PRODUCT},
    [TOKEN_PLUS_EQUALS]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_MINUS_EQUALS]    = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_STAR_EQUALS]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_SLASH_EQUALS]    = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_PLUS_PLUS]       = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_MINUS_MINUS]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_AMP]             = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_PIPE]            = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_TILDE]           = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_AMP_AMP]         = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_PIPE_PIPE]       = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_AMP_EQUALS]      = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_PIPE_EQUALS]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_BANG]            = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_PERIOD]          = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_COMMA]           = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_MORE]            = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_LESS]            = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_EQUALS]          = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_EQUALS_EQUALS]   = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_MORE_EQUALS]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_LESS_EQUALS]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_BANG_EQUALS]     = {nullptr,     nullptr,    PREC_NONE   },
    [TOKEN_IDENTIFIER]      = {nullptr,     nullptr,    PREC_NONE   },
};

ParseRule getRule(const TokenType token) {
    return rules[token];
}