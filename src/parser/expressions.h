#pragma once
#include "parser.h"

typedef enum {
    PREC_NONE,
    PREC_LIMIT, // here so we dont eat any tokens with PREC_NONE
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_SUM,
    PREC_PRODUCT,
    PREC_UNARY,
    PREC_CALL,
} ExprPrecedence;

typedef ExprNode*(*PrefixFn)(Parser*);
typedef ExprNode*(*InfixFn)(Parser*, ExprNode*);

typedef struct {
    PrefixFn prefix;
    InfixFn infix;
    ExprPrecedence precedence;
} ParseRule;

ParseRule getRule(TokenType token);

ExprNode *expression(Parser *parser);
ExprNode *parseExpr(Parser *parser, ExprPrecedence precedence);
ExprNode *parseExprPrec(Parser *parser);
ExprNode *parseExprPrecRight(Parser *parser);