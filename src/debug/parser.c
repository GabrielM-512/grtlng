#include "parser.h"

#include <stdio.h>

void printUnary(UnaryExprNode* expr) {
    switch (expr->operator) {
        case TOKEN_MINUS:
            printf(" - ");
            break;
        case TOKEN_BANG:
            printf(" ! ");
            break;
        case TOKEN_TILDE:
            printf(" ~ ");
            break;
        default:
            printf("???");
    }

    printExpr(expr->right);
}

void printExpr(TreeNode* expr) {
    printf("(");
    switch (expr->type) {

        case NODE_UNARY_EXPR:
            printUnary((UnaryExprNode*) expr);
            break;

        case NODE_NUMBER:
            printf("%f", ((NumberNode*)expr)->value);
            break;

        default:
            fprintf(stderr, "Unhandled Node type: %d\n", expr->type);
    }
    printf(")");

}
