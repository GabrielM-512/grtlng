#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>

#include "../parser.h"

double interpretExpr(TreeNode *expr) {
    switch (expr->type) {
        case NODE_UNARY_EXPR: {
            UnaryExprNode *node = (UnaryExprNode*) expr;
            switch (node->operator) {
                case TOKEN_MINUS:
                    return -interpretExpr(node->right);
                default:
            }

        } break;

        case NODE_BINARY_EXPR: {
            BinaryExprNode *node = (BinaryExprNode*) expr;
            switch (node->operator) {
                case TOKEN_PLUS:
                    return interpretExpr(node->left) + interpretExpr(node->right);
                case TOKEN_MINUS:
                    return interpretExpr(node->left) - interpretExpr(node->right);
                case TOKEN_STAR:
                    return interpretExpr(node->left) * interpretExpr(node->right);
                case TOKEN_SLASH:
                    return interpretExpr(node->left) / interpretExpr(node->right);
                default:
            }
        }

        case NODE_NUMBER:
            return ((NumberNode*) expr)->value;

        default:
            fprintf(stderr, "Non-expression node in expression AST: %d\n", expr->type);
            exit(1);

    }
}

void interpret(TreeNode *expr) {

    switch (expr->type) {
        case NODE_UNARY_EXPR:
        case NODE_NUMBER:
        case NODE_BINARY_EXPR:
            printf("\n%f\n", interpretExpr(expr));
            break;
        default:
            fprintf(stderr, "Unhandled Node type: %d\n", expr->type);
    }
}

void interpretProgram(ArrayList *program) {
    for (u32 i = 0; i < program->size; i++) {
        interpret(ArrayListRead(program, i, TreeNode*));
    }
}