#include "parser.h"

#include <stdio.h>
#include <string.h>

#include "debugInfos.h"

void printExpr(ExprNode* expr);
static void printStmt(StmtNode *stmt);

void printUnary(ExprUnaryNode* expr) {
    char *operatorName = getTokenSymbol(expr->operator);

    switch (expr->operator) {
        case TOKEN_MINUS:
        case TOKEN_BANG:
        case TOKEN_TILDE:
            break;
        default:
            fprintf(stderr, "Invalid unary expression operator %s", operatorName);
    }

    printf("%.*s", (int) strlen(operatorName) - 2, operatorName + 1);

    printExpr(expr->right);
}

void printBinary(ExprBinaryNode *expr) {
    printExpr(expr->left);

    char *operatorName = getTokenSymbol(expr->operator);
    printf(" %.*s ", (int) strlen(operatorName) - 2, operatorName + 1);

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

        case EXPR_CALL: {
            ExprCallNode *node = (ExprCallNode*) expr;
            printExpr(node->target);

            for (u32 i = 0; i < node->args.length; i++) {
                printExpr(ArrayListRead(&node->args, i, Expr*)->expr);
            }
        }
        break;

        case EXPR_INC_DEC: {
#define PRINT_INC_DEC() (printf(node->dir ? "++" : "--"))
            ExprIncDecNode *node = (ExprIncDecNode*) expr;
            if (node->time) PRINT_INC_DEC();
            printExpr(node->target);
            if (!node->time) PRINT_INC_DEC();
#undef PRINT_INC_DEC
        }
        break;

        default:
            fprintf(stderr, "    Unhandled Expression Node type: %hhu [debug/parser.c]\n", expr->type);
    }
    printf(")");

}

void printVarDec(StmtVarDeclNode *stmt) {
    printf("    Declare Variable '%s' of type %s ", stmt->name, getTokenType(stmt->varType));
    if (stmt->value == nullptr) printf("without value");
    else {
        printf("with value = ");
        printExpr(stmt->value->expr);
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
    printExpr(stmt->condition->expr);
    printf(" THEN\n");
    printStmt(stmt->thenBranch);

    if (stmt->elseBranch == nullptr) printf("    NO ELSE");
    else {
        printf("    ELSE\n");
        printStmt(stmt->elseBranch);
    }
    printf("    END IF\n");
}

void printWhile(StmtWhileNode *stmt) {
    printf("    WHILE ");
    printExpr(stmt->condition->expr);
    printf(" DO\n");
    printStmt(stmt->body);
    printf("    END WHILE");
}

static void printStmt(StmtNode *stmt) {
    switch (stmt->type) {
        case STMT_EXPR:
            printf("    [EXPR] ");
            printExpr(((StmtExprNode*)stmt)->expr->expr);
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
                printExpr(((StmtReturnNode*) stmt)->value->expr);
            }
            break;
        case STMT_IF:
            printIf((StmtIfNode*) stmt);
            break;
        case STMT_PRINT:
            printf("    print ");
            printExpr(((StmtPrintNode*) stmt)->value->expr);
            break;
        case STMT_WHILE:
            printWhile((StmtWhileNode*) stmt);
            break;
        default:
            fprintf(stderr, "    Unhandled Statement Node type: %hhu [debug/parser.c]\n", stmt->type);
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