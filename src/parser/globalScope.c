#include "globalScope.h"

#include "parser.h"
#include "parseUtils.h"
#include "scoping.h"
#include "expressions.h"
#include "statements.h"

#include "../error.h"
#include "../debug/debugInfos.h"
#include "../util/ArrayList.h"

StmtNode *functionDeclaration(Parser *parser, char *name, TokenType returnType) {

    ArrayList *parameters = ArrayListNew(sizeof(StmtVarDeclNode*));

    // left parenthesis has already been consumed

    if (isTypeIdent(parser)) {
        // parse parameters
        do {
            if (!matchTypeIdent(parser)) {
                parseErrorAtCurrent(parser, "Expected type identifier after comma, got %s instead", getTokenSymbol(parser->current.type));
                // skip remaining parameters
                while (!check(parser, TOKEN_RIGHT_PAREN) && !check(parser, TOKEN_EOF)) advance(parser);
                break;
            }

            TokenType type = parser->previous.type;

            consume(parser, TOKEN_IDENTIFIER, " after parameter type");

            char *paramName = parser->previous.data;

            StmtVarDeclNode *parameter = ALLOC_NODE(StmtVarDeclNode);

            parameter->header.type = STMT_VAR_DEC;
            parameter->name = paramName;
            parameter->value = nullptr;
            parameter->varType = type;

            ArrayListAdd(parameters, &parameter);

        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, " after function parameters");
    if (!match(parser, TOKEN_LEFT_BRACE)) {
        parseErrorAtCurrent(parser, "Functions must have a block as their body");
    }
    StmtBlockNode *body = (StmtBlockNode*) blockStmt(parser);

    StmtFunction *function = ALLOC_NODE(StmtFunction);

    function->header.type = STMT_FUN_DEC;
    function->name = name;

    function->returns = returnType;

    function->body = body;

    function->parameters = parameters;

    return (StmtNode*) function;
}

StmtNode *variableDeclaration(Parser *parser, char *name, TokenType dataType) {
    StmtVarDeclNode *node = ALLOC_NODE(StmtVarDeclNode);

    node->header.type = STMT_VAR_DEC;
    node->name = name;
    node->varType = dataType;

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

StmtNode *globalDeclaration(Parser *parser) {

    if (!matchTypeIdent(parser)) {
        parseErrorAtCurrent(parser, "Expected Function or Variable declaration");
        return nullptr;
    }

    TokenType dataType = parser->previous.type;

    if (!consume(parser, TOKEN_IDENTIFIER, " after declaration type")) {
        return nullptr;
    }

    char *name = parser->previous.data;

    if (varExists(parser, name)) {
        parseError(parser, "Global symbol \"%s\" declared twice", name);
    }

    if (match(parser, TOKEN_LEFT_PAREN)) return functionDeclaration(parser, name, dataType);

    // it's a variable
    return variableDeclaration(parser, name, dataType);

}