#include "parser.h"

#include <stdio.h>
#include <string.h>

#include "debugInfos.h"

void printExpr(ExprNode* expr);
static void printStmt(StmtNode *stmt);

void printUnary(ExprUnaryNode* expr) {
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

void printBinary(ExprBinaryNode *expr) {
    printExpr(expr->left);

    printf(" %.*s ", (int) strlen(getTokenSymbol(expr->operator)) - 2, getTokenSymbol(expr->operator) + 1);

    printExpr(expr->right);
}

void printAssignment(ExprVarAssignNode *expr) {
    printExpr(expr->target);
    printf(" = ");
    printExpr(expr->value);
}

void printExpr(ExprNode *expr) {

    printf("(");
    switch (expr->type) {

        case EXPR_UNARY:
            printUnary((ExprUnaryNode*) expr);
            break;

        case EXPR_BINARY:
            printBinary((ExprBinaryNode*) expr);
            break;

        case EXPR_NUMBER:
            printf("%f", ((ExprNumberNode*) expr)->value);
            break;

        case EXPR_VAR:
            printf("%s", ((ExprVarNode*) expr)->name);
            break;

        case EXPR_VAR_ASSIGN:
            printAssignment((ExprVarAssignNode*) expr);
            break;

        case EXPR_CALL:
            printExpr(((ExprCallNode*) expr)->target);
            for (u32 i = 0; i < ((ExprCallNode*) expr)->args.length; i++) {
                printExpr(ArrayListRead(&((ExprCallNode*) expr)->args, i, ExprNode*));
            }

            printf(")");
            break;

        default:
            fprintf(stderr, "    Unhandled Expression Node type: %d [debug/parser.c]\n", expr->type);
    }
    printf(")");

}

void printVarDec(StmtVarDeclNode *stmt) {
    printf("    Declare Variable '%s' of type %s ", stmt->name, getTokenType(stmt->varType));
    if (stmt->value == nullptr) printf("without value");
    else {
        printf("with value = ");
        printExpr(stmt->value);
    }

}

void printBlock(StmtBlockNode *block) {
    printf("    Begin block\n");
    for (u32 i = 0; i < block->content.length; i++) {
        printStmt(ArrayListRead(&block->content, i, StmtNode*));
    }
    printf("    End block");
}

void printIf(StmtIfNode *stmt) {
    printf("    IF ");
    printExpr(stmt->condition);
    printf(" THEN\n");
    printStmt(stmt->thenBranch);

    if (stmt->elseBranch == nullptr) printf("    NO ELSE");
    else {
        printf("    ELSE\n");
        printStmt(stmt->elseBranch);
    }
}

void printWhile(StmtWhileNode *stmt) {
    printf("    WHILE ");
    printExpr(stmt->condition);
    printf(" DO\n");
    printStmt(stmt->body);
    printf("    END");
}

static void printStmt(StmtNode *stmt) {
    switch (stmt->type) {
        case STMT_EXPR:
            printf("    [EXPR] ");
            printExpr(((StmtExprNode*)stmt)->expr);
            break;
        case STMT_VAR_DEC:
            printVarDec((StmtVarDeclNode*) stmt);
            break;
        case STMT_BLOCK:
            printBlock((StmtBlockNode*) stmt);
            break;
        case STMT_RETURN:
            printf("    Return ");
            if (((StmtReturnNode*) stmt)->value == nullptr) printf("without value");
            else {
                printf("with value ");
                printExpr(((StmtReturnNode*) stmt)->value);
            }
            break;
        case STMT_IF:
            printIf((StmtIfNode*) stmt);
            break;
        case STMT_PRINT:
            printf("    print ");
            printExpr(((StmtPrintNode*) stmt)->value);
            break;
        case STMT_WHILE:
            printWhile((StmtWhileNode*) stmt);
            break;
        default:
            fprintf(stderr, "    Unhandled Statement Node type: %d [debug/parser.c]\n", stmt->type);
    }
    putchar('\n');
}

void printProgram(ParseResult program) {
    StmtVarDeclNode* vars[program.tree.length];
    u32 varCount = 0;

    StmtFunction* funcs[program.tree.length];
    u32 funcCount = 0;

    for (u32 i = 0; i < program.tree.length; i++) {
        StmtNode *node = ArrayListRead(&program.tree, i, StmtNode*);

        switch (node->type) {
            case STMT_VAR_DEC: {
                vars[varCount++] = (StmtVarDeclNode*) node;
                break;
            }
            case STMT_FUN_DEC: {
                funcs[funcCount++] = (StmtFunction*) node;
                break;
            }
            default:
                // unreachable
        }
    }

    printf("init:\n");
    for (u32 i = 0; i < varCount; i++) {
        printStmt((StmtNode*)vars[i]);
        putchar('\n');
    }
    putchar('\n');

    for (u32 i = 0; i < funcCount; i++) {
        StmtFunction current = *funcs[i];
        printf("%s:\n", current.name);
        printBlock(current.body);
        printf("\n\n");
    }
}