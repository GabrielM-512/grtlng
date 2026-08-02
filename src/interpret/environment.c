#include "environment.h"

#include <stdio.h>
#include <stdlib.h>

#include "interpreter.h"

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

void createVar(char *name, const Value *value) {
    if (!HashMapSet(interpreter.env->vars, name, value)) {
        fprintf(stderr, "Fatal Interpreter Error: Variable \"%s\" already exists on declaration\n", name);
        exit(-1);
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
    exit(-1);

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
    exit(-1);

}