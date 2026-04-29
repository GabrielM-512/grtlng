#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>

#include "../parser.h"

typedef enum {
    VALUE_i16,
} ValueTypes;

typedef struct {
    ValueTypes type;
    union {
        i16 i16;
    } value;
} Value;

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

        default:
            fprintf(stderr, "Non-expression node in expression AST: %d\n", expr->type);
            exit(1);

    }
}

void interpretExpr(ExprNode *expr) {
    switch (expr->type) {
        case EXPR_UNARY_EXPR:
        case EXPR_NUMBER:
        case EXPR_BINARY_EXPR:
            printf("%f", interpretNumExpr(expr));
            break;
        case EXPR_ERROR:
            printf("Error expression");
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
    printf("========== INTERPRETER OUTPUT ==========\n");
    for (u32 i = 0; i < program->size; i++) {
        interpret(ArrayListRead(program, i, StmtNode*));
        printf("\n");
    }
}