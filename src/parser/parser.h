#pragma once

#include "../util/ArenaAllocator.h"
#include "../util/ArrayList.h"

#include "../lexer.h"


typedef enum : u8 {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_NUMBER,
    EXPR_VAR,
    EXPR_CALL,

    EXPR_VAR_ASSIGN,
    EXPR_INC_DEC,

} ExprNodeType;

typedef struct {
    ExprNodeType type;
    u32 token;
} ExprNode;

typedef struct {
    ExprNode *expr;
    u32 token;
    ArrayList prefix;
    ArrayList suffix;
} Expr;


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
    ExprNode *target;
    ArrayList args;
} ExprCallNode;

typedef struct {
    ExprNode header;
    ExprNode *target;
    ExprNode *value;
} ExprVarAssignNode;

typedef struct {
    ExprNode header;
    ExprNode *target;
    bool dir; // true = increment, false = decrement
    bool time; // true = before, false = after
} ExprIncDecNode;


typedef enum : u8 {
    STMT_VAR_DEC,
    STMT_FUN_DEC,
    STMT_EXPR,
    STMT_BLOCK,
    STMT_RETURN,
    STMT_IF,
    STMT_PRINT,
    STMT_WHILE,
} StmtNodeType;

typedef struct {
    StmtNodeType type;
    u32 token;
} StmtNode;


typedef struct {
    StmtNode header;
    TokenType varType;
    char* name;
    Expr *value;
} StmtVarDeclNode;

typedef struct {
    StmtNode header;
    Expr *expr;
} StmtExprNode;

typedef struct {
    StmtNode header;
    ArrayList content;
} StmtBlockNode;

typedef struct {
    StmtNode header;
    Expr *value;
} StmtReturnNode;

typedef struct {
    StmtNode header;
    Expr *condition;
    StmtNode *thenBranch;
    StmtNode *elseBranch;
} StmtIfNode;

typedef struct {
    StmtNode header;
    Expr *value;
} StmtPrintNode;

typedef struct {
    StmtNode header;
    Expr *condition;
    StmtNode *body;
} StmtWhileNode;


typedef struct {
    StmtNode header;
    TokenType returns;
    char *name;
    StmtBlockNode *body;
    ArrayList parameters;
} StmtFunction;


typedef struct {
    ArenaAllocator* data;
    ArrayList tree;
    ExprCallNode main;
} ParseResult;

typedef struct Scope Scope;

typedef struct {
    ParseResult program;
    ArrayList *Tokens;
    u32 token; // always points to current
    Token current, previous;
    bool inGlobalPhase;
    bool hadError, panicMode;
    const char *source;
    Scope *currentScope;
} Parser;


ParseResult parseAll(Parser *parser, ArrayList *tokens, const char* source);