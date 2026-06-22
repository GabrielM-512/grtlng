#include "statements.h"

#include <stdio.h>

#include "expressions.h"
#include "parser.h"
#include "parseUtils.h"
#include "scoping.h"

#include "../error.h"

/*
     SSSS   TTTTT     A     TTTTT   EEEEE   M   M   EEEEE   N   N   TTTTT    SSSS
    S         T      A A      T     E       MM MM   E       NN  N     T     S
     SSS      T      AAA      T     EEEEE   M M M   EEEEE   N N N     T      SSS
        S     T     A   A     T     E       M   M   E       N  NN     T         S
    SSSS      T     A   A     T     EEEEE   M   M   EEEEE   N   N     T     SSSS
 */

StmtNode *localVarDeclStmt(Parser *parser) {
    StmtVarDeclNode *node = ALLOC_NODE(StmtVarDeclNode);

    node->varType = parser->previous.type;
    node->header.type = STMT_VAR_DEC;

    if (!consume(parser, TOKEN_IDENTIFIER, " after variable type")) {
        return nullptr;
    }

    if (match(parser, TOKEN_LEFT_PAREN)) {
        parseError(parser, "Unexpected '(' in local variable declaration");
        fprintf(stderr, "Hint: Function declarations are only permitted in the global scope\n\n\n");
        return nullptr;
    }

    node->name = parser->previous.data;

    Variable var = {node->varType};

    createCurrentScopeVar(parser, node->name, var);

    node->value = nullptr;

    // if instant assignment
    if (match(parser, TOKEN_EQUALS)) {
        node->value = expression(parser);
        consume(parser, TOKEN_SEMICOLON, " after variable assignment");
    } else {
        consume(parser, TOKEN_SEMICOLON, " after variable declaration");
    }

    return (StmtNode*) node;
}

StmtNode *exprStmt(Parser *parser) {
    StmtExprNode *node = ALLOC_NODE(StmtExprNode);
    node->header.type = STMT_EXPR;

    node->expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, " after Expression");

    return (StmtNode*) node;
}

StmtNode *returnStmt(Parser *parser) {
    // todo: check what values are allowed (void vs. number etc)
    // todo: check whether all paths properly return
    StmtReturn *node = ALLOC_NODE(StmtReturn);

    node->header.type = STMT_RETURN;

    node->value = nullptr;

    if (!check(parser, TOKEN_SEMICOLON)) node->value = expression(parser);

    consume(parser, TOKEN_SEMICOLON, " after return statement");

    return (StmtNode*) node;
}

StmtNode *blockStmt(Parser *parser) {

    StmtBlockNode *node = ALLOC_NODE(StmtBlockNode);

    node->header.type = STMT_BLOCK;
    node->content = ArrayListNew(sizeof(StmtNode*));

    beginScope(parser);

    while (!match(parser, TOKEN_RIGHT_BRACE)) {

        if (match(parser, TOKEN_EOF)) {
            parseError(parser, "Unterminated block");
            break;
        }

        StmtNode *next;

        if (matchTypeIdent(parser)) next = localVarDeclStmt(parser);
        else if (match(parser, TOKEN_RETURN)) next = returnStmt(parser);
        else next = parseStmt(parser);

        ArrayListAdd(node->content, &next);
    }

    endScope(parser);

    return (StmtNode*) node;

}

StmtNode *parseStmt(Parser *parser) {
    StmtNode* node;

    if (match(parser, TOKEN_LEFT_BRACE)) node = blockStmt(parser);
    else node = exprStmt(parser);


    if (parser->panicMode) synchronise(parser);

    return node;
}