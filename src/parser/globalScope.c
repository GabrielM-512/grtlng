#include "globalScope.h"

#include "parser.h"
#include "parseUtils.h"
#include "scoping.h"
#include "expressions.h"

#include "../error.h"
#include "../util/ArrayList.h"

#define ALLOC_NODE(type) (ArenaAlloc(parser->program.data, sizeof(type)))

// todo: also skips next function declaration after the current error
void globalSynchronise(Parser *parser) {
    parser->panicMode = false;

    while (!match(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LEFT_BRACE)) {
            i16 level = 1;
            while (level > 0) {
                if (match(parser, TOKEN_EOF)) break; // error will be handled later
                if (match(parser, TOKEN_LEFT_BRACE)) level++;
                else if (match(parser, TOKEN_RIGHT_BRACE)) level--;
                else advance(parser);
            }
            return;
        }
        advance(parser);
    }
}

StmtNode *globalDeclaration(Parser *parser) {

    if (!isTypeIdent(parser)) {
        parseErrorAtCurrent(parser, "Expected Function or Variable declaration");
    }

    TokenType dataType = parser->previous.type;

    if (!consume(parser, TOKEN_IDENTIFIER, " after declaration type")) {
        return nullptr;
    }

    char *name = parser->previous.data;

    if (match(parser, TOKEN_LEFT_PAREN)) {
        // it's a function
        if (HashMapHas(&parser->program.functions, name)) {
            parseError(parser, "Function \"%s\" has already been declared", name);
        }
        if (varExists(parser, name)) {
            parseError(parser, "Function \"%s\" has already been declared as a global variable", name);
        }

        // check for parameters here

        consume(parser, TOKEN_RIGHT_PAREN, " after function parameters");

        StmtFunction *function = ALLOC_NODE(StmtFunction);

        function->header.type = STMT_FUN_DEC;
        function->name = name;

        function->body = nullptr;

        return (StmtNode*) function;
    }

    // it's a variable

    StmtVarDeclNode *node = ALLOC_NODE(StmtVarDeclNode);

    node->header.type = STMT_VAR_DEC;
    node->name = parser->previous.data;
    node->varType = dataType;

    Variable var = {dataType};

    if (HashMapHas(&parser->program.functions, node->name)) {
        parseError(parser, "Global variable \"%s\" has already been declared as a function");
    }

    createCurrentScopeVar(parser, node->name, var);

    node->value = nullptr;

    // if instant assignment
    if (match(parser, TOKEN_EQUALS)) {
        node->value = parseExprPrecRight(parser);
        consume(parser, TOKEN_SEMICOLON, " after variable assignment");
    } else {
        consume(parser, TOKEN_SEMICOLON, " after variable declaration");
    }

    return (StmtNode*) node;
}

ArrayList *parseGlobals(Parser *parser) {
    ArrayList *functions = ArrayListNew(sizeof(FunctionDeclaration));

    while (!match(parser, TOKEN_EOF)) {
        StmtNode *node = globalDeclaration(parser);
        if (node == nullptr) {
            globalSynchronise(parser);
            continue;
        }
        switch (node->type) {
            case STMT_VAR_DEC:
                // add to program
                ArrayListAdd(parser->program.tree, &node);
                break;
            case STMT_FUN_DEC: {
                StmtFunction *funNode = (StmtFunction*) node;

                // subtract 1 because parser->token points to current, not to previous
                FunctionDeclaration fun = {funNode->name, parser->token - 1};

                // add to queue
                ArrayListAdd(functions, &fun);

                // add to known functions
                HashMapSet(&parser->program.functions, fun.name, funNode);


                // skip function block to be handled later
                while (!match(parser, TOKEN_LEFT_BRACE)) advance(parser);

                u16 level = 1;

                while (level > 0) {
                    if (match(parser, TOKEN_EOF)) break; // error will be handled later
                    if (match(parser, TOKEN_LEFT_BRACE)) level++;
                    else if (match(parser, TOKEN_RIGHT_BRACE)) level--;
                    else advance(parser);
                }

                break;
            }

            default:
                break;
        }
    }

    return functions;
}