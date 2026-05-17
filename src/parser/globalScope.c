#include "globalScope.h"

#include "parser.h"
#include "parseUtils.h"
#include "scoping.h"
#include "expressions.h"

#include "../error.h"
#include "../util/ArrayList.h"

#define ALLOC_NODE(type) (ArenaAlloc(parser->program.data, sizeof(type)))

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
        parseError(parser, "Expected Function or Variable declaration");
    }

    TokenType type = parser->previous.type;

    if (!consume(parser, TOKEN_IDENTIFIER, " after declaration type")) {
        return nullptr;
    }

    char *name = parser->previous.data;

    if (match(parser, TOKEN_LEFT_PAREN)) {
        // its a function
        if (HashMapHas(&parser->program.functions, name)) {
            parseError(parser, "Function \"%s\" already declared", name);
        }

        // check for parameters here
        StmtFunction *function = ALLOC_NODE(StmtFunction);

        function->header.type = STMT_FUN_DEC;
        function->name = name;

        function->body = nullptr;

        return (StmtNode*) function;
    }

    // its a variable

    StmtVarDeclNode *node = ALLOC_NODE(StmtVarDeclNode);

    node->header.type = STMT_VAR_DEC;
    node->name = parser->previous.data;

    Variable var = {node->varType};

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

                // add to queue
                FunctionDeclaration fun = {funNode->name, parser->token};
                ArrayListAdd(functions, &fun);

                // add to known functions
                HashMapSet(&parser->program.functions, funNode->name, &funNode);


                // skip function block to be handled later
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