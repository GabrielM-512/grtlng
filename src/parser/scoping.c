#include "scoping.h"

#include <stdlib.h>

#include "parseUtils.h"
#include "../error.h"

/*
     SSSS     CCC    OOO    PPPP     III    N   N    GGG
    S        C      O   O   P   P    III    NN  N   G
     SSS    C       O   O   PPPP     III    N N N   G  GG
        S    C      O   O   P        III    N  NN   G   G
    SSSS      CCC    OOO    P        III    N   N    GGG
 */

void beginScope(Parser *parser) {
    // we actually malloc Scopes so we can properly free them when we don't need them any more
    Scope *new = malloc(sizeof(Scope));

    new->enclosing = parser->currentScope;
    HashMapInit(&new->variables, sizeof(Symbol));

    parser->currentScope = new;
}

void endScope(Parser *parser) {
    Scope *previous = parser->currentScope->enclosing;

    HashMapFree(&parser->currentScope->variables);
    free(parser->currentScope);

    parser->currentScope = previous;
}

Scope *findScope(const Parser *parser, char *name) {
    Scope *scope = parser->currentScope;

    while (true) {
        // it exists
        if (HashMapHas(&scope->variables, name)) {
            return scope;
        }

        // it doesnt exist
        if (scope->enclosing == nullptr) return nullptr;

        // keep iterating
        scope = scope->enclosing;
    }
}

bool symbolExists(const Parser *parser, char* name) {
    return findScope(parser, name) == nullptr ? false : true;
}

bool symbolInCurrentScope(const Parser *parser, char* name) {
    return HashMapHas(&parser->currentScope->variables, name);
}

void createSymbol(Parser *parser, char *name, Symbol var) {
    HashMapSet(&parser->currentScope->variables, name, &var);
}

void createCurrentScopeSymbol(Parser *parser, char *name, Symbol var) {
    if (symbolInCurrentScope(parser, name)) parseError(parser, "Variable \"%s\" already declared in current scope", name);
    else createSymbol(parser, name, var);
}

void activateSymbol(Parser *parser, char *name) {
    Symbol var = getSymbol(parser, name);
    var.initialised = true;
    Scope *scope = findScope(parser, name);

    HashMapSet(&scope->variables, name, &var);
}

Symbol getSymbol(Parser *parser, char *name) {
    Symbol var = {
        .type = TYPE_VAR,
        .initialised = false,
        .as = {nullptr}
    };

    Scope *scope = findScope(parser, name);

    if (scope != nullptr) {
        HashMapGet(&scope->variables, name, &var);
    }

    return var;

}
