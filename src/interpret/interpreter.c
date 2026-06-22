#include "interpreter.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../parser/parser.h"
#include "value.h"
#include "../util/HashMap.h"

typedef struct Environment {
    struct Environment *enclosing;
    HashMap *vars;
} Environment;

typedef struct {
    Environment *env;
    Environment *global;
    HashMap functions;
} Interpreter;

Interpreter interpreter;

void interpret(StmtNode *stmt);

void startEnvironment() {
    Environment *env = malloc(sizeof(Environment));

    env->enclosing = interpreter.env;
    env->vars = malloc(sizeof(HashMap));

    HashMapInit(env->vars, sizeof(Value));

    interpreter.env = env;
}

void endEnvironment() {
    Environment *old = interpreter.env;

    interpreter.env = old->enclosing;

    HashMapFree(old->vars);
    free(old);
}

static void createVar(char *name, const Value *value) {
    if (!HashMapSet(interpreter.env->vars, name, value)) {
        fprintf(stderr, "Fatal Interpreter Error: Variable \"%s\" already exists on declaration\n", name);
        exit(1);
    }
}

void setVar(char *name, const Value *value) {
    Environment *env = interpreter.env;
    while (true) {
        if (!HashMapHas(env->vars, name)) {
            if (env->enclosing == nullptr) break;

            env = env->enclosing;
            continue;
        }

        HashMapSet(env->vars, name, value);
        return;

    }

    fprintf(stderr, "Fatal Interpreter Error: Unknown Variable \"%s\"\n", name);
    exit(1);

}

Value getVar(char *name) {
    Value val;

    Environment *env = interpreter.env;

    while (true) {
        if (!HashMapHas(env->vars, name)) {
            if (env->enclosing == nullptr) break;

            env = env->enclosing;
            continue;
        }

        HashMapGet(env->vars, name, &val);
        return val;

    }

    fprintf(stderr, "Fatal Interpreter Error: Unknown Variable \"%s\"\n", name);
    exit(1);

}


f64 interpretNumExpr(ExprNode *expr) {
    switch (expr->type) {
        case EXPR_UNARY_EXPR: {
            ExprUnaryNode *node = (ExprUnaryNode*) expr;
            switch (node->operator) {
                case TOKEN_MINUS:
                    return -interpretNumExpr(node->right);
                default:
            }

        } break;

        case EXPR_BINARY_EXPR: {
            ExprBinaryNode *node = (ExprBinaryNode*) expr;
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
            return ((ExprNumberNode*) expr)->value;

        case EXPR_VAR:
            return getVar(((ExprVarNode*) expr)->name).value;

        case EXPR_VAR_ASSIGN: {
            ExprVarAssignNode *node = (ExprVarAssignNode*) expr;
            switch (node->target->type) {
                case EXPR_VAR: {
                    ExprVarNode *target = (ExprVarNode*) node->target;
                    Value val = {interpretNumExpr(node->value)};
                    setVar(target->name, &val);
                    return val.value;
                }
                default:
                    INTERN_ERROR_LOCATION();
                    fprintf(stderr, "Tried assigning to non-variable: %d", node->target->type);
                    exit(1);
            }
        }


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
        case EXPR_VAR_ASSIGN:
            interpretNumExpr(expr);
            break;
        case EXPR_CALL: {
            ExprCallNode *node = (ExprCallNode*) expr;

            StmtFunction function;
            HashMapGet(&interpreter.functions, node->target, &function);

            ArrayList *params = ArrayListNew(sizeof(Value));

            for (u32 i = 0; i < node->args->length; i++) {
                Value val;
                val.value = interpretNumExpr(ArrayListRead(node->args, i, ExprNode*));
                ArrayListAdd(params, &val);
            }

            Environment *old = interpreter.env;
            interpreter.env = interpreter.global;


            startEnvironment();

            for (u32 i = 0; i < node->args->length; i++) {
                createVar(ArrayListRead(function.parameters, i, Parameter).name, &ArrayListRead(params, i, Value));
            }

            StmtBlockNode *body = function.body;
            interpret((StmtNode*) body);

            endEnvironment();

            interpreter.env = old;

            break;
        }

        default:
            fprintf(stderr, "Unhandled Expression Node type: %d [interpret/interpreter.c]\n", expr->type);
    }
}

void interpret(StmtNode *stmt) {

    switch (stmt->type) {
        case STMT_EXPR:
            interpretExpr(((StmtExprNode*) stmt)->expr);
            printf("\n");
            break;
        case STMT_VAR_DEC: {
            StmtVarDeclNode *node = (StmtVarDeclNode*) stmt;

            Value val;

            if (node->value != nullptr) {
                val.value = interpretNumExpr(node->value);
            }

            createVar(node->name, &val);
            break;
        }
        case STMT_BLOCK: {
            StmtBlockNode *block = (StmtBlockNode*) stmt;
            startEnvironment();

            for (u32 i = 0; i < block->content->length; i++) {
                interpret(ArrayListRead(block->content, i, StmtNode*));
            }

            endEnvironment();
            break;
        }
        default:
            fprintf(stderr, "Unhandled Statement Node type: %d [interpret/interpreter.c]\n", stmt->type);
    }
}

void interpretProgram(ParseResult program) {
    usleep(100000);

    // create starting environment
    startEnvironment();
    interpreter.global = interpreter.env;

    interpreter.functions = program.functions;

    printf("========== INTERPRETER OUTPUT ==========\n");
    for (u32 i = 0; i < program.tree->length; i++) {
        interpret(ArrayListRead(program.tree, i, StmtNode*));
    }
}