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
    HashMapInit(&new->variables, sizeof(Variable));

    parser->currentScope = new;
}

void endScope(Parser *parser) {
    Scope *previous = parser->currentScope->enclosing;

    HashMapFree(&parser->currentScope->variables);
    free(parser->currentScope);

    parser->currentScope = previous;
}

bool varExists(const Parser *parser, char* name) {
    Scope *scope = parser->currentScope;

    while (true) {
        // it exists
        if (HashMapHas(&scope->variables, name)) return true;

        // it doesnt exist
        if (scope->enclosing == nullptr) break;

        // keep iterating
        scope = scope->enclosing;
    }

    return false;
}

bool varInCurrentScope(const Parser *parser, char* name) {
    return HashMapHas(&parser->currentScope->variables, name);
}

void createVar(Parser *parser, char *name, Variable var) {
    HashMapSet(&parser->currentScope->variables, name, &var);
}

void createCurrentScopeVar(Parser *parser, char *name, Variable var) {
    if (varInCurrentScope(parser, name)) parseError(parser, "Variable \"%s\" already declared in current scope", name);
    else createVar(parser, name, var);
}

Variable getVar(Parser *parser, char *name) {
    Variable var = {
        TOKEN_ERROR
    };

    Scope *scope = parser->currentScope;

    while (true) {
        // it exists
        if (HashMapHas(&scope->variables, name)) {
            HashMapGet(&scope->variables, name, &var);
            break;
        }

        // it doesnt exist
        if (scope->enclosing == nullptr) break;

        // keep iterating
        scope = scope->enclosing;
    }


    return var;
}
