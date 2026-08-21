#pragma once

#include "parser.h"

#define ALLOC_NODE(type) (ArenaAlloc(parser->program.data, sizeof(type)))

typedef void (*exprFn)(Parser*, Expr*);
typedef void (*ExprNodeFn)(ExprNode*, void*);

void advance(Parser* parser);

bool consume(Parser *parser, TokenType type, const char *message);
bool match(Parser *parser, TokenType type);
bool check (const Parser *parser, TokenType type);

bool isTypeIdent(Parser *parser);
bool matchTypeIdent(Parser *parser);

void synchronise(Parser *parser);

void skipBlock(Parser *parser);

void reachExpr(Parser *parser, exprFn fn);
void recurseExpr(ExprNode* n, ExprNodeFn fn, void *data);

typedef enum {
    TYPE_VAR,
    TYPE_FUNC
} SymbolType;

typedef struct {
    SymbolType type;
    bool initialised;
    union {
        StmtVarDeclNode *var;
        StmtFunction *func;
    } as;
} Symbol;

#define FUNC_SYMBOL(symbol) ((Symbol) {.type = TYPE_FUNC, true, {.func = (symbol)}})
#define VAR_SYMBOL(symbol) ((Symbol) {.type = TYPE_VAR, false, {.var = (symbol)}})

#define AS_FUNC(symbol) ((symbol).as.func)
#define AS_VAR(symbol) ((symbol).as.var)

#define IS_FUNC(symbol) ((symbol).type == TYPE_FUNC)
#define IS_VAR(symbol) ((symbol).type == TYPE_VAR)