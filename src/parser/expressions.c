#include "expressions.h"

#include "parser.h"
#include "scoping.h"
#include "parseUtils.h"

#include "../error.h"
#include "../debug/debugInfos.h"

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

    node->operator = parser->previous.type;
    node->left = left;

    node->right = parseExprPrec(parser);

    return (ExprNode*) node;
}

void parseArgs(Parser *parser, ExprCallNode *call, u32 arity) {
    u32 paramsPassed = 0;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            ExprNode *param = expression(parser);
            ArrayListAdd(call->args, &param);
            paramsPassed++;
        } while (match(parser, TOKEN_COMMA));

    }

    if (arity != paramsPassed)
        parseError(parser, "Function \"%s\" expects %u arguments, %u were passed instead", call->target, arity, paramsPassed);

}

ExprNode *call(Parser *parser, ExprNode *left) {
    if (parser->inGlobalPhase) {
        parseError(parser, "No functions may be called during global variable initialisation");
    }
    ExprCallNode *node = ALLOC_NODE(ExprCallNode);

    node->header.type = EXPR_CALL;
    node->args = ArrayListNew(sizeof(ExprNode*));

    u32 functionArity;

    switch (left->type) {
        case EXPR_VAR:
            node->target = ((ExprVarNode*) left)->name;

            functionArity = getFunction(parser, node->target).parameters->length;

            break;
        default:
            parseError(parser, "Invalid assignment target");
            functionArity = 0;
    }

    parseArgs(parser, node, functionArity);

    consume(parser, TOKEN_RIGHT_PAREN, " after function arguments");

    return (ExprNode*) node;
}

ExprNode *exprUnary(Parser *parser) {
    ExprUnaryNode *node = ALLOC_NODE(ExprUnaryNode);

    node->header.type = EXPR_UNARY;
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
    ExprNode *node = expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "");
    return node;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
ExprNode *variable(Parser *parser) {
    ExprVarNode *node = ALLOC_NODE(ExprVarNode);
    node->header.type = EXPR_VAR;
    node->name = parser->previous.data;

    if (!varExists(parser, node->name) && !HashMapHas(&parser->program.functions, node->name)) {
        parseError(parser, "Unknown variable or function identifier \"%s\"", node->name);
    }

    if (varExists(parser, node->name)) {
        if (!getVar(parser, node->name).initialised) {
            parseError(parser, "Variable \"%s\" may not use itself in its own declaration", node->name);
        }
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

ExprNode *relativeAssignment(Parser *parser, ExprNode *left) {
    // desugar to regular assignment
    ExprVarAssignNode *node = ALLOC_NODE(ExprVarAssignNode);
    node->header.type = EXPR_VAR_ASSIGN;
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

ExprNode *expression(Parser *parser) {
    return parseExpr(parser, PREC_LIMIT);
}

// TODO: not

ParseRule rules [TOKEN_LAST] = {
    [TOKEN_EOF]             = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_ERROR]           = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_NUM]             = {number,      nullptr,               PREC_NONE       },
    [TOKEN_STRING]          = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_SEMICOLON]       = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_LEFT_PAREN]      = {grouping,    call,                  PREC_CALL       },
    [TOKEN_RIGHT_PAREN]     = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_LEFT_BRACE]      = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_RIGHT_BRACE]     = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_LEFT_BRACKET]    = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_RIGHT_BRACKET]   = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_PLUS]            = {nullptr,     exprBinary,            PREC_SUM        },
    [TOKEN_MINUS]           = {exprUnary,   exprBinary,            PREC_SUM        },
    [TOKEN_STAR]            = {nullptr,     exprBinary,            PREC_PRODUCT    },
    [TOKEN_SLASH]           = {nullptr,     exprBinary,            PREC_PRODUCT    },
    [TOKEN_PLUS_EQUALS]     = {nullptr,     relativeAssignment,    PREC_ASSIGNMENT },
    [TOKEN_MINUS_EQUALS]    = {nullptr,     relativeAssignment,    PREC_ASSIGNMENT },
    [TOKEN_STAR_EQUALS]     = {nullptr,     relativeAssignment,    PREC_ASSIGNMENT },
    [TOKEN_SLASH_EQUALS]    = {nullptr,     relativeAssignment,    PREC_ASSIGNMENT },
    [TOKEN_PLUS_PLUS]       = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_MINUS_MINUS]     = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_AMP]             = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_PIPE]            = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_TILDE]           = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_AMP_AMP]         = {nullptr,     exprBinary,            PREC_AND        },
    [TOKEN_PIPE_PIPE]       = {nullptr,     exprBinary,            PREC_OR         },
    [TOKEN_AMP_EQUALS]      = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_PIPE_EQUALS]     = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_BANG]            = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_DOT]             = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_COMMA]           = {nullptr,     nullptr,               PREC_NONE       },
    [TOKEN_MORE]            = {nullptr,     exprBinary,            PREC_COMPARISON },
    [TOKEN_LESS]            = {nullptr,     exprBinary,            PREC_COMPARISON },
    [TOKEN_EQUALS]          = {nullptr,     assignment,            PREC_ASSIGNMENT },
    [TOKEN_EQUALS_EQUALS]   = {nullptr,     exprBinary,            PREC_EQUALITY   },
    [TOKEN_MORE_EQUALS]     = {nullptr,     exprBinary,            PREC_COMPARISON },
    [TOKEN_LESS_EQUALS]     = {nullptr,     exprBinary,            PREC_COMPARISON },
    [TOKEN_BANG_EQUALS]     = {nullptr,     exprBinary,            PREC_EQUALITY   },
    [TOKEN_IDENTIFIER]      = {variable,    nullptr,               PREC_NONE       },
};

ParseRule getRule(const TokenType token) {
    return rules[token];
}