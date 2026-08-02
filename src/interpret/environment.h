#pragma once

#include "../util/HashMap.h"
#include "value.h"

typedef struct Environment {
    struct Environment *enclosing;
    HashMap *vars;
} Environment;

void startEnvironment();
void endEnvironment();

void createVar(char *name, const Value *value);
void setVar(char *name, const Value *value);
Value getVar(char *name);