#include "interpreter.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../parser/parser.h"
#include "value.h"
#include "../debug/debugInfos.h"
#include "../util/HashMap.h"

// TODO: make [EXPR] [FUNCTION_IDENTIFIER]; valid usable

typedef struct Environment {
    struct Environment *enclosing;
    HashMap *vars;
} Environment;

typedef struct {
    Environment *env;
    Environment *global;
    HashMap functions;
    bool returning;
    Value returnValue;
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

static Value getVar(char *name) {
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

f64 evaluate(ExprNode *expr);


f64 evaluateCall(ExprCallNode *call) {

    StmtFunction function;
    HashMapGet(&interpreter.functions, call->target, &function);

    ArrayList *params = ArrayListNew(sizeof(Value));

    for (u32 i = 0; i < call->args->length; i++) {
        Value val;
        val.value = evaluate(ArrayListRead(call->args, i, ExprNode*));
        ArrayListAdd(params, &val);
    }

    Environment *old = interpreter.env;
    interpreter.env = interpreter.global;


    startEnvironment();

    for (u32 i = 0; i < call->args->length; i++) {
        createVar(ArrayListRead(function.parameters, i, Parameter).name, &ArrayListRead(params, i, Value));
    }

    StmtBlockNode *body = function.body;
    interpret((StmtNode*) body);

    endEnvironment();

    interpreter.env = old;

    interpreter.returning = false;

    Value returns = interpreter.returnValue;

    interpreter.returnValue.value = NAN;

    return returns.value;
}

bool isTruthy(f64 val) {
    return val != 0;
}


f64 evaluate(ExprNode *expr) {
    switch (expr->type) {
        case EXPR_UNARY_EXPR: {
            ExprUnaryNode *node = (ExprUnaryNode*) expr;
            switch (node->operator) {
                case TOKEN_MINUS:
                    return -evaluate(node->right);
                default:
            }

        } break;

        case EXPR_BINARY_EXPR: {

#define MAKE_OPERATION(type, operator) case type: return evaluate(node->left) operator evaluate(node->right)

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
                    if (!isTruthy(evaluate(node->left))) return false;
                    return isTruthy(evaluate(node->right));
                case TOKEN_PIPE_PIPE:
                    if (isTruthy(evaluate(node->left))) return true;
                    return isTruthy(evaluate(node->right));
                default:
                    fprintf(stderr, "Interpreter cannot evaluate Binary Expression Token %s (%d)", getTokenSymbol(node->operator), node->operator);
                    exit(1);
            }
#undef MAKE_OPERATION
        }

        case EXPR_NUMBER:
            return ((ExprNumberNode*) expr)->value;

        case EXPR_VAR:
            return getVar(((ExprVarNode*) expr)->name).value;

        case EXPR_VAR_ASSIGN: {
            ExprVarAssignNode *node = (ExprVarAssignNode*) expr;
            switch (node->target->type) {
                case EXPR_VAR: {
                    ExprVarNode *target = (ExprVarNode*) node->target;
                    Value val = {evaluate(node->value)};
                    setVar(target->name, &val);
                    return val.value;
                }
                default:
                    INTERN_ERROR_LOCATION();
                    fprintf(stderr, "Tried assigning to non-variable: %d", node->target->type);
                    exit(1);
            }
        }

        case EXPR_CALL:
            return evaluateCall((ExprCallNode*) expr);


        default:
            fprintf(stderr, "    Unhandled Expression Node type: %d [interpret/interpreter.c]\n", expr->type);
            exit(-1);

    }
    return NAN;
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
                val.value = evaluate(node->value);
            }

            createVar(node->name, &val);
            break;
        }
        case STMT_BLOCK: {
            StmtBlockNode *block = (StmtBlockNode*) stmt;
            startEnvironment();

            for (u32 i = 0; i < block->content->length; i++) {
                interpret(ArrayListRead(block->content, i, StmtNode*));
                if (interpreter.returning) break;
            }

            endEnvironment();
            break;
        }
        case STMT_RETURN: {
            StmtReturnNode *node = (StmtReturnNode*) stmt;

            if (node->value != nullptr) interpreter.returnValue.value = evaluate(node->value);
            else interpreter.returnValue.value = NAN;

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
            printf("%f\n", evaluate(node->value));
            break;
        }
        default:
            fprintf(stderr, "    Unhandled Statement Node type: %d [interpret/interpreter.c]\n", stmt->type);
            exit(-1);
    }
}

i32 interpretProgram(ParseResult program) {
    usleep(100000);

    // create starting environment
    startEnvironment();
    interpreter.global = interpreter.env;

    interpreter.functions = program.functions;

    printf("========== INTERPRETER OUTPUT ==========\n");
    for (u32 i = 0; i < program.tree->length; i++) {
        interpret(ArrayListRead(program.tree, i, StmtNode*));
    }

    return (i32) evaluateCall(&program.main);

}