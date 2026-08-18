#include "interpreter.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../parser/parser.h"
#include "value.h"

Interpreter interpreter;

void interpret(StmtNode *stmt);

void interpretBlock(StmtBlockNode *node) {
    startEnvironment();

    for (u32 i = 0; i < node->content.length; i++) {
        interpret(ArrayListRead(&node->content, i, StmtNode*));
        if (interpreter.returning) break;
    }

    endEnvironment();
}

void interpret(StmtNode *stmt) {

    switch (stmt->type) {
        case STMT_EXPR:
            evaluate(((StmtExprNode*) stmt)->expr);
            break;
        case STMT_VAR_DEC: {
            StmtVarDeclNode *node = (StmtVarDeclNode*) stmt;

            Value val;

            if (node->value != nullptr) {
                val = evaluate(node->value);
            }

            createVar(node->name, &val);
            break;
        }
        case STMT_FUN_DEC: {
            StmtFunction *node = (StmtFunction*) stmt;
            createVar(node->name, &VALUE_FUNC(node));
            break;
        }
        case STMT_BLOCK: {
            interpretBlock((StmtBlockNode*) stmt);
            break;
        }
        case STMT_RETURN: {
            StmtReturnNode *node = (StmtReturnNode*) stmt;

            if (node->value != nullptr) interpreter.returnValue = evaluate(node->value);
            else interpreter.returnValue = VALUE_NUM(NAN);

            interpreter.returning = true;

            break;
        }
        case STMT_IF: {
            StmtIfNode *node = (StmtIfNode*) stmt;

            if (isTruthy(evaluate(node->condition))) {
                interpret(node->thenBranch);
            } else if (node->elseBranch != nullptr) {
                interpret(node->elseBranch);
            }
            break;
        }
        case STMT_PRINT: {
            StmtPrintNode *node = (StmtPrintNode*) stmt;
            printValue(evaluate(node->value));
            break;
        }
        case STMT_WHILE: {
            StmtWhileNode *node = (StmtWhileNode*) stmt;
            while (isTruthy(evaluate(node->condition))) {
                interpret(node->body);
            }
            break;
        }
        default:
            fprintf(stderr, "    Unhandled Statement Node type: %d [interpret/interpreter.c]\n", stmt->type);
            exit(-1);
    }
}

i32 interpretProgram(ParseResult program) {

    // create starting environment
    startEnvironment();
    interpreter.global = interpreter.env;

    for (u32 i = 0; i < program.tree.length; i++) {
        interpret(ArrayListRead(&program.tree, i, StmtNode*));
    }

    return (i32) AS_NUM(evaluateCall(&program.main));

}