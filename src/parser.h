#pragma once

#include "util/ArenaAllocator.h"
#include "util/ArrayList.h"

#include "lexer.h"

typedef struct {
    ArenaAllocator* data;
    ArrayList *tree;
} ParseResult;

typedef struct {
    ParseResult program;
    ArrayList *Tokens;
    u16 token;
    Token current, previous;
    bool hadError, panicMode;
    const char *source;
} Parser;


typedef enum {
    EXPR_BINARY_EXPR,
    EXPR_UNARY_EXPR,
    EXPR_NUMBER,
    EXPR_VAR

} ExprNodeType;

typedef struct {
    ExprNodeType type;
} ExprNode;


typedef struct {
    ExprNode header;
    ExprNode *left;
    ExprNode *right;
    TokenType operator;
} BinaryExprNode;

typedef struct {
    ExprNode header;
    ExprNode *right;
    TokenType operator;
} UnaryExprNode;

typedef struct {
    ExprNode header;
    double value;
} NumberNode;

typedef struct {
    ExprNode header;
    char *name;
} VarAccessNode;


typedef enum {
    STMT_VAR_DEC,
    STMT_VAR_ASSIGN,
    STMT_EXPR,
} StmtNodeType;

typedef struct {
    StmtNodeType type;
} StmtNode;


typedef struct {
    StmtNode header;
    char* name;
    ExprNode *value;
} VarAssignNode;

typedef struct {
    StmtNode header;
    TokenType varType;
    char* name;
    ExprNode *value;
} VarDeclNode;

typedef struct {
    StmtNode header;
    ExprNode *expr;
} StmtExprNode;




ParseResult parseAll(Parser *parser, ArrayList *tokens, const char* source);