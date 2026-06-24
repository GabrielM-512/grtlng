#pragma once

#include "../util/ArenaAllocator.h"
#include "../util/ArrayList.h"
#include "../util/HashMap.h"

#include "../lexer.h"


typedef enum {
    EXPR_BINARY_EXPR,
    EXPR_UNARY_EXPR,
    EXPR_NUMBER,
    EXPR_VAR,
    EXPR_CALL,

    EXPR_VAR_ASSIGN,

} ExprNodeType;

typedef struct {
    ExprNodeType type;
} ExprNode;


typedef struct {
    ExprNode header;
    ExprNode *left;
    ExprNode *right;
    TokenType operator;
} ExprBinaryNode;

typedef struct {
    ExprNode header;
    ExprNode *right;
    TokenType operator;
} ExprUnaryNode;

typedef struct {
    ExprNode header;
    double value;
} ExprNumberNode;

typedef struct {
    ExprNode header;
    char *name;
} ExprVarNode;

typedef struct {
    ExprNode header;
    char *target;
    ArrayList *args;
} ExprCallNode;

typedef struct {
    ExprNode header;
    ExprNode *target;
    ExprNode *value;
} ExprVarAssignNode;


typedef enum {
    STMT_VAR_DEC,
    STMT_FUN_DEC,
    STMT_EXPR,
    STMT_BLOCK,
    STMT_RETURN,
} StmtNodeType;

typedef struct {
    StmtNodeType type;
} StmtNode;


typedef struct {
    StmtNode header;
    TokenType varType;
    char* name;
    ExprNode *value;
} StmtVarDeclNode;

typedef struct {
    StmtNode header;
    ExprNode *expr;
} StmtExprNode;

typedef struct {
    StmtNode header;
    ArrayList *content;
} StmtBlockNode;

typedef struct {
    StmtNode header;
    ExprNode *value;
} StmtReturnNode;


typedef struct {
    StmtNode header;
    TokenType returns;
    char *name;
    StmtBlockNode *body;
    ArrayList *parameters;
} StmtFunction;


typedef struct {
    ArenaAllocator* data;
    ArrayList *tree;
    HashMap functions;
    ExprCallNode main;
} ParseResult;

struct Scope;

typedef struct {
    ParseResult program;
    ArrayList *Tokens;
    u32 token;
    Token current, previous;
    bool inGlobalPhase;
    bool hadError, panicMode;
    const char *source;
    struct Scope *currentScope;
} Parser;


typedef struct {
    TokenType type;
    char *name;
} Parameter;

ParseResult parseAll(Parser *parser, ArrayList *tokens, const char* source);