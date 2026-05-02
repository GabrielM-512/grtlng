#include "parser.h"

#include <stdio.h>

#include "debugInfos.h"

void printExpr(ExprNode* expr);

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

void printBinary(BinaryExprNode *expr) {
    printExpr(expr->left);

    switch (expr->operator) {
        case TOKEN_PLUS:
            printf(" + ");
            break;
        case TOKEN_MINUS:
            printf(" - ");
            break;
        case TOKEN_STAR:
            printf(" * ");
            break;
        case TOKEN_SLASH:
            printf(" / ");
            break;
        default:
            printf("???");
    }
    printExpr(expr->right);
}

void printExpr(ExprNode *expr) {

    printf("(");
    switch (expr->type) {

        case EXPR_UNARY_EXPR:
            printUnary((UnaryExprNode*) expr);
            break;

        case EXPR_BINARY_EXPR:
            printBinary((BinaryExprNode*) expr);
            break;

        case EXPR_NUMBER:
            printf("%f", ((NumberNode*) expr)->value);
            break;

        case EXPR_VAR:
            printf("%s", ((VarAccessNode*) expr)->name);
            break;

        case EXPR_ERROR:
            printf("Error Token");
            break;

        default:
            fprintf(stderr, "Unhandled Expression Node type: %d\n", expr->type);
    }
    printf(")");

}

void printVarDec(VarDeclNode *stmt) {
    printf("Declare Variable '%s' of type %s ", stmt->name, getTokenType(stmt->varType));
    if (stmt->value == nullptr) printf("without value");
    else {
        printf("with value = ");
        printExpr(stmt->value);
    }

}

void printStmt(StmtNode *stmt) {
    switch (stmt->type) {
        case STMT_EXPR:
            printf("[EXPR] ");
            printExpr(((StmtExprNode*)stmt)->expr);
            break;
        case STMT_VAR_DEC:
            printVarDec((VarDeclNode*) stmt);
            break;
        default:
            fprintf(stderr, "Unhandled Statement Node type: %d\n", stmt->type);
    }
}
