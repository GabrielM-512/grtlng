#include "parser.h"

#include "error.h"

#include "debug/debugInfos.h"

#define ALLOC_NODE(type) (ArenaAlloc(parser->program.data, sizeof(type)))

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
ExprNode *parseExprPrec(Parser *parser);
StmtNode *parseStmt(Parser *parser);

// externally callable function(s)

ParseResult parseAll(Parser *parser, ArrayList *tokens, const char* source) {
    parser->Tokens = tokens;
    parser->token = 0;

    parser->program.tree = ArrayListNew(sizeof(StmtNode*));
    parser->program.data = ArenaNew();

    parser->hadError = false;
    parser->panicMode = false;

    parser->source = source;

    advance(parser);

    while (!match(parser, TOKEN_EOF)) {
        StmtNode *expr = parseStmt(parser);
        ArrayListAdd(parser->program.tree, &expr);
    }

    consume(parser, TOKEN_EOF, "Expected end of file");

    return parser->program;
}

void synchronise(Parser *parser) {
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        advance(parser);
    }
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

bool check(const Parser *parser, TokenType type) {
    return parser->current.type == type;
}


bool isVarIdent(Parser *parser) {
    constexpr TokenType types[] = {TOKEN_I16, TOKEN_I32, TOKEN_U16, TOKEN_U32, TOKEN_LAST};
    u16 i = 0;
    while (types[i] != TOKEN_LAST) {
        if (match(parser, types[i])) return true;
        i++;
    }

    return false;
}

// statement functions

StmtNode *exprStmt(Parser *parser) {
    StmtExprNode *node = ALLOC_NODE(StmtExprNode);
    node->header.type = STMT_EXPR;

    node->expr = parseExpr(parser, PREC_ASSIGNMENT);
    consume(parser, TOKEN_SEMICOLON, " after Expression");

    return (StmtNode*) node;
}

StmtNode *varDeclStmt(Parser *parser) {
    VarDeclNode *node = ALLOC_NODE(VarDeclNode);

    node->varType = parser->previous.type;

    consume(parser, TOKEN_IDENTIFIER, " after variable type");

    node->header.type = STMT_VAR_DEC;
    node->name = parser->previous.data;


    node->value = nullptr;

    // if instant assignment
    if (match(parser, TOKEN_EQUALS)) {
        node->value = parseExprPrec(parser);
    }

    consume(parser, TOKEN_SEMICOLON, " after variable declaration");

    return (StmtNode*) node;
}

StmtNode *parseStmt(Parser *parser) {
    StmtNode* node;
    if (isVarIdent(parser)) node = varDeclStmt(parser);
    else node = exprStmt(parser);

    synchronise(parser);

    return node;
}

// expression functions

ExprNode *exprBinary(Parser *parser, ExprNode *left) {
    BinaryExprNode *node = ALLOC_NODE(BinaryExprNode);

    node->header.type = EXPR_BINARY_EXPR;

    node->operator = parser->previous.type;
    node->left = left;

    node->right = parseExprPrec(parser);

    return (ExprNode*) node;

}

ExprNode *exprUnary(Parser *parser) {
    UnaryExprNode *node = ALLOC_NODE(UnaryExprNode);

    node->header.type = EXPR_UNARY_EXPR;
    node->operator = parser->previous.type;

    node->right = parseExprPrec(parser);

    return (ExprNode*) node;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static ExprNode *number(Parser *parser) {
    NumberNode *node = ALLOC_NODE(NumberNode);

    node->header.type = EXPR_NUMBER;
    node->value = * (double*) parser->previous.data;

    return (ExprNode*) node;
}

ExprNode *grouping(Parser *parser) {
    ExprNode *node = parseExprPrec(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "");
    return node;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
ExprNode *variable(Parser *parser) {
    VarAccessNode *node = ALLOC_NODE(VarAccessNode);
    node->header.type = EXPR_VAR;
    node->name = parser->previous.data;
    return (ExprNode*) node;
}


ExprNode *parseExpr(Parser *parser, ExprPrecedence precedence) {
    advance(parser);

    const PrefixFn prefixRule = getRule(parser->previous.type).prefix;

    if (prefixRule == nullptr) {

        parseError(parser, "Tried starting (sub-) expression with invalid token: %s", getTokenSymbol(parser->previous.type));

        return nullptr;
    }

    ExprNode *left = prefixRule(parser);

    while (precedence < getRule(parser->current.type).precedence) {
        advance(parser);
        const InfixFn infixRule = getRule(parser->previous.type).infix;
        left = infixRule(parser, left);
    }

    return left;

}

ExprNode *parseExprPrec(Parser *parser) {
    return parseExpr(parser, getRule(parser->previous.type).precedence);
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
    [TOKEN_IDENTIFIER]      = {variable,     nullptr,    PREC_NONE  },
};

ParseRule getRule(const TokenType token) {
    return rules[token];
}