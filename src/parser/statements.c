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

StmtNode *localVarDeclStmt(Parser *parser);
StmtNode *exprStmt(Parser *parser);

StmtNode *whileStmt(Parser *parser) {
    StmtWhileNode *node = ALLOC_NODE(StmtWhileNode);

    node->header.type = STMT_WHILE;

    consume(parser, TOKEN_LEFT_PAREN, " after \"while\"");

    node->condition = expression(parser);

    consume(parser, TOKEN_RIGHT_PAREN, " after while condition");

    node->body = parseStmt(parser);

    return (StmtNode*) node;
}

StmtNode *forStmt(Parser *parser) {
    // desugaring for loops to while loops and a block
    consume(parser, TOKEN_LEFT_PAREN, " after \"for\"");

    StmtNode *initialiser = nullptr;

    if (!match(parser, TOKEN_SEMICOLON)) {
        if (matchTypeIdent(parser)) initialiser = localVarDeclStmt(parser);
        else initialiser = exprStmt(parser);
    }

    ExprNode *condition;

    if (!check(parser, TOKEN_SEMICOLON)) {
        condition = expression(parser);
    } else {
        ExprNumberNode *temp = ALLOC_NODE(ExprNumberNode);

        *temp = (ExprNumberNode) {{EXPR_NUMBER}, 1};

        condition = (ExprNode*) temp;
    }

    consume(parser, TOKEN_SEMICOLON, " after loop condition");

    ExprNode *incrementer = nullptr;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        incrementer = expression(parser);
    }

    consume(parser, TOKEN_RIGHT_PAREN, " after incrementor clause");

    StmtNode *body = parseStmt(parser);

    StmtBlockNode *block = ALLOC_NODE(StmtBlockNode);

    block->header.type = STMT_BLOCK;
    block->content = ArrayListNew(sizeof(StmtNode*));

    if (initialiser != nullptr) ArrayListAdd(block->content, &initialiser);


    StmtWhileNode *loop = ALLOC_NODE(StmtWhileNode);
    loop->header.type = STMT_WHILE;

    loop->condition = condition;
    loop->body = body;


    if (incrementer != nullptr) {
        StmtBlockNode *newBody = ALLOC_NODE(StmtBlockNode);

        newBody->header.type = STMT_BLOCK;
        newBody->content = ArrayListNew(sizeof(StmtNode*));

        ArrayListAdd(newBody->content, &body);

        StmtExprNode *incExpr = ALLOC_NODE(StmtExprNode);
        incExpr->header.type = STMT_EXPR;
        incExpr->expr = incrementer;

        ArrayListAdd(newBody->content, &incExpr);
        loop->body = (StmtNode*) newBody;
    }

    ArrayListAdd(block->content, &loop);

    return (StmtNode*) block;
}

StmtNode *ifStmt(Parser *parser) {
    StmtIfNode *node = ALLOC_NODE(StmtIfNode);

    node->header.type = STMT_IF;

    consume(parser, TOKEN_LEFT_PAREN, " after if");

    node->condition = expression(parser);

    consume(parser, TOKEN_RIGHT_PAREN, " after condition");

    node->thenBranch = parseStmt(parser);

    if (match(parser, TOKEN_ELSE)) node->elseBranch = parseStmt(parser);
    else node->elseBranch = nullptr;

    return (StmtNode*) node;
}

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

    Variable var = {node->varType, false};

    createCurrentScopeVar(parser, node->name, var);

    node->value = nullptr;

    // if instant assignment
    if (match(parser, TOKEN_EQUALS)) {
        node->value = expression(parser);
        consume(parser, TOKEN_SEMICOLON, " after variable assignment");
    } else {
        consume(parser, TOKEN_SEMICOLON, " after variable declaration");
    }

    var.initialised = true;

    createVar(parser, node->name, var);

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
    StmtReturnNode *node = ALLOC_NODE(StmtReturnNode);

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

static StmtNode *printStmt(Parser *parser) {
    StmtPrintNode *node = ALLOC_NODE(StmtPrintNode);
    node->header.type = STMT_PRINT;

    node->value = expression(parser);

    consume(parser, TOKEN_SEMICOLON, " after print statement");
    return (StmtNode*) node;
}


StmtNode *parseStmt(Parser *parser) {
    StmtNode* node;

    if (match(parser, TOKEN_LEFT_BRACE)) node = blockStmt(parser);
    else if (match(parser, TOKEN_IF)) node = ifStmt(parser);
    else if (match(parser, TOKEN_RETURN)) node = returnStmt(parser);
    else if (match(parser, TOKEN_PRINT)) node = printStmt(parser);
    else if (match(parser, TOKEN_WHILE)) node = whileStmt(parser);
    else if (match(parser, TOKEN_FOR)) node = forStmt(parser);
    else node = exprStmt(parser);


    if (parser->panicMode) synchronise(parser);

    return node;
}