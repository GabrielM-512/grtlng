#include "interpreter.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../parser.h"
#include "value.h"

typedef struct {

} Interpreter;

Interpreter interpreter;

double interpretNumExpr(ExprNode *expr) {
    switch (expr->type) {
        case EXPR_UNARY_EXPR: {
            UnaryExprNode *node = (UnaryExprNode*) expr;
            switch (node->operator) {
                case TOKEN_MINUS:
                    return -interpretNumExpr(node->right);
                default:
            }

        } break;

        case EXPR_BINARY_EXPR: {
            BinaryExprNode *node = (BinaryExprNode*) expr;
            switch (node->operator) {
                case TOKEN_PLUS:
                    return interpretNumExpr(node->left) + interpretNumExpr(node->right);
                case TOKEN_MINUS:
                    return interpretNumExpr(node->left) - interpretNumExpr(node->right);
                case TOKEN_STAR:
                    return interpretNumExpr(node->left) * interpretNumExpr(node->right);
                case TOKEN_SLASH:
                    return interpretNumExpr(node->left) / interpretNumExpr(node->right);
                default:
            }
        }
            break;

        case EXPR_NUMBER:
            return ((NumberNode*) expr)->value;

        case EXPR_VAR:
            break;

        default:
            fprintf(stderr, "Non-expression node in expression AST: %d\n", expr->type);
            exit(1);

    }
    return NAN;
}

void interpretExpr(ExprNode *expr) {
    switch (expr->type) {
        case EXPR_UNARY_EXPR:
        case EXPR_NUMBER:
        case EXPR_BINARY_EXPR:
        case EXPR_VAR:
            printf("%f", interpretNumExpr(expr));
            break;
        default:
            fprintf(stderr, "Unhandled Expression Node type: %d", expr->type);
    }
}

void interpret(StmtNode *stmt) {

    switch (stmt->type) {
        case STMT_EXPR:
            interpretExpr(((StmtExprNode*) stmt)->expr);
            break;
        case STMT_VAR_DEC:
            break;
        default:
            fprintf(stderr, "Unhandled Statement Node type: %d", stmt->type);
    }
}

void interpretProgram(ArrayList *program) {
    usleep(100000);

    printf("========== INTERPRETER OUTPUT ==========\n");
    for (u32 i = 0; i < program->size; i++) {
        interpret(ArrayListRead(program, i, StmtNode*));
        printf("\n");
    }
}