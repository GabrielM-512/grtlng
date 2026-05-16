#include "parser.h"

#include <stdlib.h>

#include "error.h"

#include "debug/debugInfos.h"
#include "util/HashMap.h"

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

typedef struct Scope {
    struct Scope *enclosing;
    HashMap variables;
} Scope;

typedef struct {
    TokenType type;
} Variable;

static void advance(Parser* parser);
void consume(Parser *parser, TokenType type, const char *message);
static bool match(Parser *parser, TokenType type);
bool check (const Parser *parser, TokenType type);

/*
     SSSS     CCC    OOO    PPPP     III    N   N    GGG
    S        C      O   O   P   P    III    NN  N   G
     SSS    C       O   O   PPPP     III    N N N   G  GG
        S    C      O   O   P        III    N  NN   G   G
    SSSS      CCC    OOO    P        III    N   N    GGG
 */

void beginScope(Parser *parser) {
    // we actually malloc Scopes so we can properly free them when we don't need them any more
    Scope *new = malloc(sizeof(Scope));

    new->enclosing = parser->currentScope;
    HashMapInit(&new->variables, sizeof(Variable));

    parser->currentScope = new;
}

void endScope(Parser *parser) {
    Scope *previous = parser->currentScope->enclosing;

    HashMapFree(&parser->currentScope->variables);
    free(parser->currentScope);

    parser->currentScope = previous;
}

static void createVar(Parser *parser, char *name, Variable var) {
    HashMapSet(&parser->currentScope->variables, name, &var);
}

bool varExists(const Parser *parser, char* name) {
    Scope *scope = parser->currentScope;

    while (true) {
        // it exists
        if (HashMapHas(&scope->variables, name)) return true;

        // it doesnt exist
        if (scope->enclosing == nullptr) break;

        // keep iterating
        scope = scope->enclosing;
    }

    return false;
}

bool varInCurrentScope(const Parser *parser, char* name) {
    return HashMapHas(&parser->currentScope->variables, name);
}

ParseRule getRule(TokenType token);

ExprNode *parseExpr(Parser *parser, ExprPrecedence precedence);
ExprNode *parseExprPrec(Parser *parser);
ExprNode *parseExprPrecRight(Parser *parser);
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

    parser->currentScope = nullptr;
    beginScope(parser);

    advance(parser);

    while (!match(parser, TOKEN_EOF)) {
        StmtNode *expr = parseStmt(parser);
        ArrayListAdd(parser->program.tree, &expr);
    }

    consume(parser, TOKEN_EOF, "Expected end of file");

    return parser->program;
}


// error recovery function
void synchronise(Parser *parser) {
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        advance(parser);
    }
}

/*
    U   U   TTTTT    III    L        III    TTTTT    III    EEEEE    SSSS
    U   U     T      III    L        III      T      III    E       S
    U   U     T      III    L        III      T      III    EEEEE    SSS
    U   U     T      III    L        III      T      III    E           S
     UUU      T      III    LLLLL    III      T      III    EEEEE   SSSS
 */

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

/*
     SSSS   TTTTT     A     TTTTT   EEEEE   M   M   EEEEE   N   N   TTTTT    SSSS
    S         T      A A      T     E       MM MM   E       NN  N     T     S
     SSS      T      AAA      T     EEEEE   M M M   EEEEE   N N N     T      SSS
        S     T     A   A     T     E       M   M   E       N  NN     T         S
    SSSS      T     A   A     T     EEEEE   M   M   EEEEE   N   N     T     SSSS
 */

bool isTypeIdent(Parser *parser) {
    constexpr TokenType types[] = {TOKEN_I16, TOKEN_I32, TOKEN_U16, TOKEN_U32};

    for (u64 i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        if (match(parser, types[i])) return true;
    }

    return false;
}

StmtNode *varDeclStmt(Parser *parser) {
    StmtVarDeclNode *node = ALLOC_NODE(StmtVarDeclNode);

    node->varType = parser->previous.type;

    consume(parser, TOKEN_IDENTIFIER, " after variable type");

    node->header.type = STMT_VAR_DEC;
    node->name = parser->previous.data;

    Variable var = {node->varType};

    if (varInCurrentScope(parser, node->name)) parseError(parser, "Variable \"%s\" already declared in current scope", node->name);
    else createVar(parser, node->name, var);


    node->value = nullptr;

    // if instant assignment
    if (match(parser, TOKEN_EQUALS)) {
        node->value = parseExprPrecRight(parser);
        consume(parser, TOKEN_SEMICOLON, " after variable assignment");
    } else {
        consume(parser, TOKEN_SEMICOLON, " after variable declaration");
    }

    return (StmtNode*) node;
}

StmtNode *exprStmt(Parser *parser) {
    StmtExprNode *node = ALLOC_NODE(StmtExprNode);
    node->header.type = STMT_EXPR;

    node->expr = parseExpr(parser, PREC_NONE);
    consume(parser, TOKEN_SEMICOLON, " after Expression");

    return (StmtNode*) node;
}

StmtNode *blockStmt(Parser *parser) {

    StmtBlockNode *node = ALLOC_NODE(StmtBlockNode);

    node->header.type = STMT_BLOCK;
    node->content = ArrayListNew(sizeof(StmtNode*));

    beginScope(parser);

    while (!match(parser, TOKEN_RIGHT_BRACE)) {
        if (match(parser, TOKEN_EOF)) parseError(parser, "Unterminated block");
        StmtNode *next;

        if (isTypeIdent(parser)) next = varDeclStmt(parser);
        else next = parseStmt(parser);
        ArrayListAdd(node->content, &next);
    }

    endScope(parser);

    return (StmtNode*) node;

}

StmtNode *parseStmt(Parser *parser) {
    StmtNode* node;
    if (match(parser, TOKEN_LEFT_BRACE)) node = blockStmt(parser);
    else node = exprStmt(parser);

    if (parser->panicMode) synchronise(parser);

    return node;
}

/*
    EEEEE   X   X   PPPP    RRRR    EEEEE    SSSS    SSSS    III     OOO    N   N    SSSS
    E        X X    P   P   R   R   E       S       S        III    O   O   NN  N   S
    EEEEE     X     PPPP    RRRR    EEEEE    SSS     SSS     III    O   O   N N N    SSS
    E        X X    P       R  R    E           S       S    III    O   O   N  NN       S
    EEEEE   X   X   P       R   R   EEEEE   SSSS    SSSS     III     OOO    N   N   SSSS
 */

ExprNode *exprBinary(Parser *parser, ExprNode *left) {
    ExprBinaryNode *node = ALLOC_NODE(ExprBinaryNode);

    node->header.type = EXPR_BINARY_EXPR;

    node->operator = parser->previous.type;
    node->left = left;

    node->right = parseExprPrec(parser);

    return (ExprNode*) node;

}

ExprNode *exprUnary(Parser *parser) {
    ExprUnaryNode *node = ALLOC_NODE(ExprUnaryNode);

    node->header.type = EXPR_UNARY_EXPR;
    node->operator = parser->previous.type;

    node->right = parseExprPrec(parser);

    return (ExprNode*) node;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static ExprNode *number(Parser *parser) {
    ExprNumberNode *node = ALLOC_NODE(ExprNumberNode);

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
    ExprVarNode *node = ALLOC_NODE(ExprVarNode);
    node->header.type = EXPR_VAR;
    node->name = parser->previous.data;

    if (!varExists(parser, node->name)) {
        parseError(parser, "Unknown variable \"%s\"", node->name);
    }

    return (ExprNode*) node;
}


/*
 * An assignment acts as an expression which also has a side effect.
 * The value of the expression is the same as the value assigned to the variable.
 * E.g. 5 + (a = 5) == 10
 */

ExprNode *assignment(Parser *parser, ExprNode *left) {
    ExprVarAssignNode *node = ALLOC_NODE(ExprVarAssignNode);

    node->header.type = EXPR_VAR_ASSIGN;

    // TODO: maybe its possible to mark the start of the l-value instead of the = on the error message?
    if (left->type != EXPR_VAR) {
        parseError(parser, "Invalid assignment target");
    }

    node->target = left;
    node->value = parseExprPrecRight(parser);

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

ExprNode *parseExprPrecRight(Parser *parser) {
    return parseExpr(parser, getRule(parser->previous.type).precedence - 1);
}


ParseRule rules [TOKEN_LAST] = {
    [TOKEN_EOF]             = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_ERROR]           = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_NUM]             = {number,      nullptr,    PREC_NONE       },
    [TOKEN_STRING]          = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_SEMICOLON]       = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_LEFT_PAREN]      = {grouping,    nullptr,    PREC_NONE       },
    [TOKEN_RIGHT_PAREN]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_LEFT_BRACE]      = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_RIGHT_BRACE]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_LEFT_BRACKET]    = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_RIGHT_BRACKET]   = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_PLUS]            = {nullptr,     exprBinary, PREC_SUM        },
    [TOKEN_MINUS]           = {exprUnary,   exprBinary, PREC_SUM        },
    [TOKEN_STAR]            = {nullptr,     exprBinary, PREC_PRODUCT    },
    [TOKEN_SLASH]           = {nullptr,     exprBinary, PREC_PRODUCT    },
    [TOKEN_PLUS_EQUALS]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_MINUS_EQUALS]    = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_STAR_EQUALS]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_SLASH_EQUALS]    = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_PLUS_PLUS]       = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_MINUS_MINUS]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_AMP]             = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_PIPE]            = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_TILDE]           = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_AMP_AMP]         = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_PIPE_PIPE]       = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_AMP_EQUALS]      = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_PIPE_EQUALS]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_BANG]            = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_DOT]             = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_COMMA]           = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_MORE]            = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_LESS]            = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_EQUALS]          = {nullptr,     assignment, PREC_ASSIGNMENT },
    [TOKEN_EQUALS_EQUALS]   = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_MORE_EQUALS]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_LESS_EQUALS]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_BANG_EQUALS]     = {nullptr,     nullptr,    PREC_NONE       },
    [TOKEN_IDENTIFIER]      = {variable,    nullptr,    PREC_NONE       },
};

ParseRule getRule(const TokenType token) {
    return rules[token];
}