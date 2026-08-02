#include "value.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "../debug/debugInfos.h"

#include "interpreter.h"

Value evaluateExpr(ExprNode *expr);

void printValue(Value val) {
    switch (val.type) {
        case VAL_NUM:
            printf("%f", AS_NUM(val));
            break;
        case VAL_FUNC: {
            StmtFunction *function = AS_FUNC(val);
            printf("<fn \"%s\" at %p>", function->name, function);
            break;
        }

        default:
    }
    putchar('\n');
}

bool isTruthy(Value val) {
    switch (val.type) {
        case VAL_NUM:
            return AS_NUM(val) != 0;
        case VAL_FUNC:
            return true;
        default:
            // unreachable
    }
    return true;
}


Value evaluateCall(ExprCallNode *call) {

    Value target = evaluateExpr(call->target);
    if (!IS_FUNC(target)) {
        fprintf(stderr, "Tried to call non-function");
        exit(-1);
    }
    StmtFunction function = *AS_FUNC(target);

    // temp code
    if (call->args.length != function.parameters.length) {
        fprintf(stderr, "Fatal Interpreter Error: Function \"%s\" expected %u arguments, got %u instead\n",
            function.name, function.parameters.length, call->args.length);
        exit(-1);
    }

    Value params[call->args.length];

    for (u32 i = 0; i < call->args.length; i++) {
        params[i] = evaluate(ArrayListRead(&call->args, i, Expr*));
    }

    Environment *old = interpreter.env;
    interpreter.env = interpreter.global;


    startEnvironment();

    for (u32 i = 0; i < call->args.length; i++) {
        createVar(ArrayListRead(&function.parameters, i, StmtVarDeclNode*)->name, &params[i]);
    }

    interpretBlock(function.body);

    endEnvironment();

    interpreter.env = old;

    interpreter.returning = false;

    Value returns = interpreter.returnValue;

    interpreter.returnValue = VALUE_NAN;

    return returns;
}

Value evaluateExpr(ExprNode *expr) {
    switch (expr->type) {
        case EXPR_UNARY: {
            ExprUnaryNode *node = (ExprUnaryNode*) expr;
            switch (node->operator) {
                case TOKEN_MINUS:
                    return VALUE_NUM(- AS_NUM(evaluateExpr(node->right)));
                case TOKEN_BANG:
                    return VALUE_BOOL(!isTruthy(evaluateExpr(node->right)));
                case TOKEN_PLUS:
                    return evaluateExpr(node->right);
                default:
                    fprintf(stderr, "Interpreter cannot evaluate Unary Expression Token %s (#%d)", getTokenSymbol(node->operator), node->operator);
                    exit(-1);
            }
        }

        case EXPR_BINARY: {

#define MAKE_OPERATION(type, operator) case type: return VALUE_NUM(AS_NUM(evaluateExpr(node->left)) operator AS_NUM(evaluateExpr(node->right)))

            ExprBinaryNode *node = (ExprBinaryNode*) expr;
            switch (node->operator) {
                MAKE_OPERATION(TOKEN_PLUS, +);
                MAKE_OPERATION(TOKEN_MINUS, -);
                MAKE_OPERATION(TOKEN_STAR, *);
                MAKE_OPERATION(TOKEN_SLASH, /);

                MAKE_OPERATION(TOKEN_MORE, >);
                MAKE_OPERATION(TOKEN_LESS, <);
                MAKE_OPERATION(TOKEN_MORE_EQUALS, >=);
                MAKE_OPERATION(TOKEN_LESS_EQUALS, <=);
                MAKE_OPERATION(TOKEN_EQUALS_EQUALS, ==);
                MAKE_OPERATION(TOKEN_BANG_EQUALS, !=);

                case TOKEN_AMP_AMP:
                    if (!isTruthy(evaluateExpr(node->left))) return VALUE_FALSE;
                    return VALUE_BOOL(isTruthy(evaluateExpr(node->right)));
                case TOKEN_PIPE_PIPE:
                    if (isTruthy(evaluateExpr(node->left))) return VALUE_TRUE;
                    return VALUE_BOOL(isTruthy(evaluateExpr(node->right)));
                default:
                    fprintf(stderr, "Interpreter cannot evaluate Binary Expression Token %s (%d)", getTokenSymbol(node->operator), node->operator);
                    exit(-1);
            }
#undef MAKE_OPERATION
        }

        case EXPR_NUMBER:
            return VALUE_NUM(((ExprNumberNode*) expr)->value);

        case EXPR_VAR:
            return getVar(((ExprVarNode*) expr)->name);

        case EXPR_VAR_ASSIGN: {
            ExprVarAssignNode *node = (ExprVarAssignNode*) expr;
            switch (node->target->type) {
                case EXPR_VAR: {
                    ExprVarNode *target = (ExprVarNode*) node->target;
                    Value val = evaluateExpr(node->value);
                    setVar(target->name, &val);
                    return val;
                }
                default:
                    INTERN_ERROR_LOCATION();
                    fprintf(stderr, "Tried assigning to non-variable: %d", node->target->type);
                    exit(-1);
            }
        }

        case EXPR_CALL:
            return evaluateCall((ExprCallNode*) expr);


        default:
            fprintf(stderr, "    Unhandled Expression Node type: %d [interpret/value.c]\n", expr->type);
            exit(-1);

    }
    return VALUE_NUM(NAN);
}

Value evaluate(Expr *expr) {
    return evaluateExpr(expr->expr);
}