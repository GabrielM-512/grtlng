#pragma once

#include "parser.h"
#include "parseUtils.h"
#include "../util/HashMap.h"

void beginScope(Parser *parser);
void endScope(Parser *parser);

bool varExists(const Parser *parser, char* name);
bool varInCurrentScope(const Parser *parser, char* name);

void createVar(Parser *parser, char *name, Variable var);
void createCurrentScopeVar(Parser *parser, char *name, Variable var);

Variable getVar(Parser *parser, char *name);

typedef struct Scope {
    struct Scope *enclosing;
    HashMap variables;
} Scope;