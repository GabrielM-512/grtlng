#include "parser.h"

#include <stdarg.h>
#include <stdio.h>

#include "debug/debugInfos.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_SUM,
    PREC_PRODUCT
} ExprPrecedence;

typedef ExprNode*(*PrefixFn)(Parser*);
typedef ExprNode*(*InfixFn)(Parser*, ExprNode*);

typedef struct {
    PrefixFn prefix;
    InfixFn infix;
    ExprPrecedence precedence;
} ParseRule;

static void advance(Parser* parser);
void consume(Parser *parser, TokenType type, const char *message);
static bool match(Parser *parser, TokenType type);
bool check (const Parser *parser, TokenType type);

ParseRule getRule(TokenType token);

ExprNode *parseExpr(Parser *parser, ExprPrecedence precedence);
StmtNode *parseStmt(Parser *parser);

// externally callable function(s)

ParseResult parseAll(Parser *parser, ArrayList *tokens) {
    parser->Tokens = tokens;
    parser->token = 0;

    parser->program.tree = ArrayListNew(sizeof(StmtNode*));
    parser->program.data = ArenaNew();

    parser->hadError = false;
    parser->panicMode = false;

    advance(parser);

    while (!match(parser, TOKEN_EOF)) {
        StmtNode *expr = parseStmt(parser);
        ArrayListAdd(parser->program.tree, &expr);
    }

    consume(parser, TOKEN_EOF, "Expected end of file");

    return parser->program;
}

// error handling

void parseErrorAt(Parser *parser, const Token* token, const char* message, va_list args) {
    if (parser->panicMode) {
        return;
    }

    parser->panicMode = true;
    parser->hadError = true;

    fprintf(stderr, "Encountered error on line %d: ", token->line);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    // synchronise here

}

void parseErrorAtCurrent(Parser *parser, const char* message, ...) {
    va_list args;
    va_start(args, message);

    parseErrorAt(parser, &parser->current, message, args);

    va_end(args);
}

void parseError(Parser *parser, const char* message, ...) {
    va_list args;
    va_start(args, message);
    parseErrorAt(parser, &parser->previous, message, args);
    va_end(args);
}

void expectedGotInstead(Parser *parser, const char* location, TokenType expected, TokenType got) {
    parseError(parser, "Expected %s%s, got %s instead", getTokenSymbol(expected), location, getTokenSymbol(got));
}

// utils

static bool isAtEnd(const Parser *parser) {
    return parser->Tokens->size <= parser->token;
}

static void advance(Parser *parser) {
    parser->previous = parser->current;
    while (true) {
        if (isAtEnd(parser)) {
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
    expectedGotInstead(parser, message, type, parser->current.type);
}

static bool match(Parser *parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

bool check (const Parser *parser, TokenType type) {
    return parser->current.type == type;
}


bool isVarIdent(Parser *parser) {
    const TokenType types[] = {TOKEN_I16, TOKEN_LAST};
    u16 i = 0;
    bool matched = false;

    while (types[i] != TOKEN_LAST) {
        if (types[i++] != parser->current.type) continue;


        matched = true;
        advance(parser);
        break;

    }

    return matched;
}

// statement functions

StmtNode *exprStmt(Parser *parser) {
    StmtExprNode *node = ArenaAlloc(parser->program.data, sizeof(StmtExprNode));
    node->header.type = STMT_EXPR;

    node->expr = parseExpr(parser, PREC_ASSIGNMENT);
    consume(parser, TOKEN_SEMICOLON, " after Expression");

    return (StmtNode*) node;
}

StmtNode *varDeclStmt(Parser *parser) {
    VarDeclNode *node = ArenaAlloc(parser->program.data, sizeof(VarDeclNode));

    node->varType = parser->previous.type;

    consume(parser, TOKEN_IDENTIFIER, " after variable type");

    node->header.type = STMT_VAR_DEC;
    node->name = parser->previous.data;


    node->value = nullptr;

    // if instant assignment
    if (match(parser, TOKEN_EQUALS)) {
        node->value = parseExpr(parser, PREC_ASSIGNMENT);
    }

    consume(parser, TOKEN_SEMICOLON, " after variable declaration");

    return (StmtNode*) node;
}

StmtNode *parseStmt(Parser *parser) {
    if (isVarIdent(parser)) return varDeclStmt(parser);
    return exprStmt(parser);
}

// expression functions

ExprNode *exprBinary(Parser *parser, ExprNode *left) {
    BinaryExprNode *node = ArenaAlloc(parser->program.data, sizeof(BinaryExprNode));

    node->header.type = EXPR_BINARY_EXPR;

    node->operator = parser->previous.type;
    node->left = left;

    node->right = parseExpr(parser, getRule(parser->previous.type).precedence);

    return (ExprNode*) node;

}

ExprNode *exprUnary(Parser *parser) {
    UnaryExprNode *node = ArenaAlloc(parser->program.data, sizeof(UnaryExprNode));

    node->header.type = EXPR_UNARY_EXPR;
    node->operator = parser->previous.type;

    node->right = parseExpr(parser, getRule(parser->previous.type).precedence);

    return (ExprNode*) node;
}

static ExprNode *number(Parser *parser) {
    NumberNode *node = ArenaAlloc(parser->program.data, sizeof(NumberNode));

    node->header.type = EXPR_NUMBER;
    node->value = * (double*) parser->previous.data;

    return (ExprNode*) node;
}

ExprNode *grouping(Parser *parser) {
    ExprNode *node = parseExpr(parser, getRule(parser->previous.type).precedence);
    consume(parser, TOKEN_RIGHT_PAREN, "");
    return node;
}


ExprNode *parseExpr(Parser *parser, ExprPrecedence precedence) {
    advance(parser);

    const PrefixFn prefixRule = getRule(parser->previous.type).prefix;

    if (prefixRule == nullptr) {

        parseError(parser, "Tried starting expression with unexpected Token: %s", getTokenSymbol(parser->previous.type));

        ExprNode *node = ArenaAlloc(parser->program.data, sizeof(ExprNode));
        node->type = EXPR_ERROR;

        return node;
    }

    ExprNode *left = prefixRule(parser);

    while (precedence < getRule(parser->current.type).precedence) {
        advance(parser);
        const InfixFn infixRule = getRule(parser->previous.type).infix;
        left = infixRule(parser, left);
    }

    return left;

}


ParseRule rules [TOKEN_LAST] = {
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
    [TOKEN_DOT]             = {nullptr,     nullptr,    PREC_NONE   },
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