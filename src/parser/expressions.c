#include "expressions.h"

#include "parser.h"
#include "scoping.h"
#include "parseUtils.h"

#include "../error.h"

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
ExprNode *parseExprPrec(Parser *parser);
ExprNode *parseExprPrecRight(Parser *parser);
ExprNode *parseExprNode(Parser *parser);

/*
    EEEEE   X   X   PPPP    RRRR    EEEEE    SSSS    SSSS    III     OOO    N   N    SSSS
    E        X X    P   P   R   R   E       S       S        III    O   O   NN  N   S
    EEEEE     X     PPPP    RRRR    EEEEE    SSS     SSS     III    O   O   N N N    SSS
    E        X X    P       R  R    E           S       S    III    O   O   N  NN       S
    EEEEE   X   X   P       R   R   EEEEE   SSSS    SSSS     III     OOO    N   N   SSSS
 */

ExprNode *exprBinary(Parser *parser, ExprNode *left) {
    ExprBinaryNode *node = ALLOC_NODE(ExprBinaryNode);

    node->header.type = EXPR_BINARY;
    node->header.token = parser->token - 1;

    node->operator = parser->previous.type;
    node->left = left;

    node->right = parseExprPrec(parser);

    return (ExprNode*) node;
}

void parseArgs(Parser *parser, ExprCallNode *call) {

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            Expr *param = expression(parser);
            ArrayListAdd(&call->args, &param);
        } while (match(parser, TOKEN_COMMA));
    }
}

ExprNode *call(Parser *parser, ExprNode *left) {

    ExprCallNode *node = ALLOC_NODE(ExprCallNode);

    node->header.type = EXPR_CALL;
    node->header.token = parser->token - 1;;

    node->target = left;

    ArrayListInit(&node->args, sizeof(Expr*));
    parseArgs(parser, node);

    consume(parser, TOKEN_RIGHT_PAREN, " after function arguments");

    return (ExprNode*) node;
}

ExprNode *exprUnary(Parser *parser) {
    Token operator = parser->previous;
    u32 position = parser->token - 1;
    ExprNode *operand = parseExprPrecRight(parser);

    if (operator.type == TOKEN_MINUS && operand->type == EXPR_NUMBER) {
        ExprNumberNode *number = (ExprNumberNode*) operand;
        number->header.token = position;
        number->value = -number->value;
        return (ExprNode*) number;
    }

    ExprUnaryNode *node = ALLOC_NODE(ExprUnaryNode);

    node->header.type = EXPR_UNARY;
    node->header.token = position;
    node->operator = operator.type;
    node->right = operand;

    return (ExprNode*) node;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static ExprNode *number(Parser *parser) {
    ExprNumberNode *node = ALLOC_NODE(ExprNumberNode);

    node->header.type = EXPR_NUMBER;
    node->header.token = parser->token - 1;
    node->value = * (double*) parser->previous.data;

    return (ExprNode*) node;
}

ExprNode *grouping(Parser *parser) {
    ExprNode *node = parseExprNode(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "");
    return node;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
ExprNode *variable(Parser *parser) {
    ExprVarNode *node = ALLOC_NODE(ExprVarNode);
    node->header.type = EXPR_VAR;
    node->header.token = parser->token - 1;
    node->name = parser->previous.data;

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
    node->header.token = parser->token - 1;

    if (left->type != EXPR_VAR) {
        parseError(parser, "Invalid assignment target");
    }

    node->target = left;
    node->value = parseExprPrecRight(parser);

    return (ExprNode*) node;
}

ExprNode *relativeAssignment(Parser *parser, ExprNode *left) {
    // desugar to regular assignment
    ExprVarAssignNode *node = ALLOC_NODE(ExprVarAssignNode);
    node->header.type = EXPR_VAR_ASSIGN;
    node->header.token = parser->token - 1;
    node->target = left;

    ExprBinaryNode *calculation = ALLOC_NODE(ExprBinaryNode);
    calculation->header.type = EXPR_BINARY;
    calculation->left = left;

    switch (parser->previous.type) {
        case TOKEN_PLUS_EQUALS:
            calculation->operator = TOKEN_PLUS;
            break;
        case TOKEN_MINUS_EQUALS:
            calculation->operator = TOKEN_MINUS;
            break;
        case TOKEN_STAR_EQUALS:
            calculation->operator = TOKEN_STAR;
            break;
        case TOKEN_SLASH_EQUALS:
            calculation->operator = TOKEN_SLASH;
            break;

        default:
    }

    calculation->right = parseExprPrec(parser);

    node->value = (ExprNode*) calculation;

    return (ExprNode*) node;
}

ExprNode *incrementDecrement(Parser *parser, ExprNode *target, bool dir, u32 position) {
    ExprIncDecNode *node = ALLOC_NODE(ExprIncDecNode);
    node->header.type = EXPR_INC_DEC;
    node->header.token = position;

    const bool postfix = parser->previous.type == TOKEN_PLUS_PLUS || parser->previous.type == TOKEN_MINUS_MINUS;

    node->target = target;
    node->dir = dir;
    node->time = !postfix;

    return (ExprNode*) node;
}

ExprNode *prefixIncDec(Parser *parser) {
    const bool dir = parser->previous.type == TOKEN_PLUS_PLUS;
    const u32 position = parser->token - 1;

    ExprNode *target = parseExprPrec(parser);

    return incrementDecrement(parser, target, dir, position);
}

ExprNode *postfixIncDec(Parser *parser, ExprNode *target) {
    const bool dir = parser->previous.type == TOKEN_PLUS_PLUS;

    return incrementDecrement(parser, target, dir, parser->token - 1);
}


ExprNode *parseExpr(Parser *parser, ExprPrecedence precedence) {
    advance(parser);

    const PrefixFn prefixRule = getRule(parser->previous.type).prefix;

    if (prefixRule == nullptr) {

        parseError(parser, "Expected expression");

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

ExprNode *parseExprNode(Parser *parser) {
    return parseExpr(parser, PREC_LIMIT);
}

Expr *expression(Parser *parser) {
    Expr *expr = ALLOC_NODE(Expr);
    expr->token = parser->token;
    expr->expr = parseExprNode(parser);
    ArrayListInit(&expr->prefix, sizeof(ExprNode*));
    ArrayListInit(&expr->suffix, sizeof(ExprNode*));
    return expr;
}

// TODO: bitwise
ParseRule rules [TOKEN_LAST] = {
    [TOKEN_EOF]           = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_ERROR]         = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_NUM]           = {.prefix = number,       .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_STRING]        = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_SEMICOLON]     = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_LEFT_PAREN]    = {.prefix = grouping,     .infix = call,               .precedence = PREC_CALL       },
    [TOKEN_RIGHT_PAREN]   = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_LEFT_BRACE]    = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_RIGHT_BRACE]   = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_LEFT_BRACKET]  = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_RIGHT_BRACKET] = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_PLUS]          = {.prefix = exprUnary,    .infix = exprBinary,         .precedence = PREC_SUM        },
    [TOKEN_MINUS]         = {.prefix = exprUnary,    .infix = exprBinary,         .precedence = PREC_SUM        },
    [TOKEN_STAR]          = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_PRODUCT    },
    [TOKEN_SLASH]         = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_PRODUCT    },
    [TOKEN_PLUS_EQUALS]   = {.prefix = nullptr,      .infix = relativeAssignment, .precedence = PREC_ASSIGNMENT },
    [TOKEN_MINUS_EQUALS]  = {.prefix = nullptr,      .infix = relativeAssignment, .precedence = PREC_ASSIGNMENT },
    [TOKEN_STAR_EQUALS]   = {.prefix = nullptr,      .infix = relativeAssignment, .precedence = PREC_ASSIGNMENT },
    [TOKEN_SLASH_EQUALS]  = {.prefix = nullptr,      .infix = relativeAssignment, .precedence = PREC_ASSIGNMENT },
    [TOKEN_PLUS_PLUS]     = {.prefix = prefixIncDec, .infix = postfixIncDec,      .precedence = PREC_CALL       },
    [TOKEN_MINUS_MINUS]   = {.prefix = prefixIncDec, .infix = postfixIncDec,      .precedence = PREC_CALL       },
    [TOKEN_AMP]           = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_PIPE]          = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_TILDE]         = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_AMP_AMP]       = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_AND        },
    [TOKEN_PIPE_PIPE]     = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_OR         },
    [TOKEN_AMP_EQUALS]    = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_PIPE_EQUALS]   = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_BANG]          = {.prefix = exprUnary,    .infix = nullptr,            .precedence = PREC_UNARY      },
    [TOKEN_DOT]           = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_COMMA]         = {.prefix = nullptr,      .infix = nullptr,            .precedence = PREC_NONE       },
    [TOKEN_MORE]          = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_COMPARISON },
    [TOKEN_LESS]          = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_COMPARISON },
    [TOKEN_EQUALS]        = {.prefix = nullptr,      .infix = assignment,         .precedence = PREC_ASSIGNMENT },
    [TOKEN_EQUALS_EQUALS] = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_EQUALITY   },
    [TOKEN_MORE_EQUALS]   = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_COMPARISON },
    [TOKEN_LESS_EQUALS]   = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_COMPARISON },
    [TOKEN_BANG_EQUALS]   = {.prefix = nullptr,      .infix = exprBinary,         .precedence = PREC_EQUALITY   },
    [TOKEN_IDENTIFIER]    = {.prefix = variable,     .infix = nullptr,            .precedence = PREC_NONE       },
};

ParseRule getRule(const TokenType token) {
    return rules[token];
}