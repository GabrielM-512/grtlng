#pragma once

#include "parser.h"
#include "parseUtils.h"
#include "../util/HashMap.h"

void beginScope(Parser *parser);
void endScope(Parser *parser);

bool symbolExists(const Parser *parser, char* name);
bool symbolInCurrentScope(const Parser *parser, char* name);

void createSymbol(Parser *parser, char *name, Symbol var);
void createCurrentScopeSymbol(Parser *parser, char *name, Symbol var);

void activateSymbol(Parser *parser, char *name);

Symbol getSymbol(Parser *parser, char *name);

typedef struct Scope {
    Scope *enclosing;
    HashMap variables;
} Scope;