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
    Token current;
    bool hadError, panicMode;
} Parser;


typedef enum {
    NODE_ERROR,
    NODE_BINARY_EXPR,
    NODE_UNARY_EXPR,
} NodeType;

typedef struct {
    NodeType type;
} TreeNode;


/*typedef struct {
    TreeNode header;
    TreeNode *left;
    TreeNode *right;
    TokenType operator;
} BinaryExprNode;

typedef struct {
    TreeNode header;
    TreeNode *right;
    TokenType operator;
} UnaryExprNode;*/


ParseResult parseAll(Parser *parser, ArrayList *tokens);