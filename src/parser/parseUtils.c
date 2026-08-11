#include "parseUtils.h"

#include <stdio.h>

#include "../error.h"

/*
    U   U   TTTTT    III    L        III    TTTTT    III    EEEEE    SSSS
    U   U     T      III    L        III      T      III    E       S
    U   U     T      III    L        III      T      III    EEEEE    SSS
    U   U     T      III    L        III      T      III    E           S
     UUU      T      III    LLLLL    III      T      III    EEEEE   SSSS
 */

static bool isAtEnd(const Parser *parser) {
    return parser->Tokens->length <= parser->token;
}

void advance(Parser *parser) {
    parser->previous = parser->current;
    while (true) {
        if (isAtEnd(parser)) {
            parser->current = ArrayListRead(parser->Tokens, parser->Tokens->length - 1, Token);
        } else {
            parser->current = ArrayListRead(parser->Tokens, parser->token, Token);
            parser->token++;
        }

        if (parser->current.type != TOKEN_ERROR) break;

        parseErrorAtCurrent(parser, parser->current.data);
    }
}

bool consume(Parser *parser, TokenType type, const char *message) {
    if (parser->current.type == type) {
        if (!isAtEnd(parser)) advance(parser);
        return true;
    }
    expectedGotInstead(parser, message, type, parser->current.type);
    return false;
}

bool match(Parser *parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

bool check(const Parser *parser, TokenType type) {
    return parser->current.type == type;
}

bool isTypeIdent(Parser *parser) {
    constexpr TokenType types[] = {TOKEN_I16, TOKEN_I32, TOKEN_I64, TOKEN_U16, TOKEN_U32, TOKEN_U64, TOKEN_VOID};

    for (u64 i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        if (check(parser, types[i])) return true;
    }

    return false;
}

bool matchTypeIdent(Parser *parser) {
    bool result = isTypeIdent(parser);
    if (result) advance(parser);
    return result;
}

void synchronise(Parser *parser) {
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {

        if (parser->previous.type == TOKEN_SEMICOLON) return;

        switch (parser->current.type) {
            case TOKEN_RIGHT_BRACE:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_PRINT:
                return;
            default:
        }

        advance(parser);
    }
}

void traverseStmt(Parser *parser, exprFn fn, StmtNode* node) {
    switch (node->type) {
        case STMT_EXPR:
            fn(parser, ((StmtExprNode*) node)->expr);
            break;
        case STMT_BLOCK: {
            StmtBlockNode *block = (StmtBlockNode*) node;
            for (u32 i = 0; i < block->content.length; i++) {
                StmtNode *current = ArrayListRead(&block->content, i, StmtNode*);
                traverseStmt(parser, fn, current);
            }
            break;
        }
        case STMT_FUN_DEC: {
            StmtFunction *func = (StmtFunction*) node;
            traverseStmt(parser, fn, (StmtNode*) func->body);
            break;
        }
        case STMT_IF: {
            StmtIfNode *ifNode = (StmtIfNode*) node;
            fn(parser, ifNode->condition);
            traverseStmt(parser, fn, ifNode->thenBranch);
            if (ifNode->elseBranch != nullptr) traverseStmt(parser, fn, ifNode->elseBranch);
            break;
        }
        case STMT_PRINT: {
            fn(parser, ((StmtPrintNode*) node)->value);
            break;
        }
        case STMT_RETURN: {
            fn(parser, ((StmtReturnNode*) node)->value);
            break;
        }
        case STMT_VAR_DEC: {
            StmtVarDeclNode *dec = (StmtVarDeclNode*) node;
            if (dec->value != nullptr) fn(parser, dec->value);
            break;
        }
        case STMT_WHILE: {
            StmtWhileNode *whileNode = (StmtWhileNode*) node;
            fn(parser, whileNode->condition);
            traverseStmt(parser, fn, whileNode->body);
            break;
        }
    }
}

void reachExpr(Parser *parser, exprFn fn) {
    for (u32 i = 0; i < parser->program.tree.length; i++) {
        StmtNode *node = ArrayListRead(&parser->program.tree, i, StmtNode*);
        traverseStmt(parser, fn, node);
    }
}

void recurseExpr(ExprNode* n, ExprNodeFn fn, void *data) {
    switch (n->type) {
        case EXPR_BINARY: {
            ExprBinaryNode *node = (ExprBinaryNode*) n;
            fn(node->left, data);
            fn(node->right, data);
            break;
        }
        case EXPR_CALL: {
            break;
            INTERN_ERROR_LOCATION();
            fprintf(stderr, "Called recurseExpr() on a call expression\n");
        }
        case EXPR_VAR:
        case EXPR_NUMBER: {
            // nothing to recurse to
            break;
        }
        case EXPR_UNARY: {
            ExprUnaryNode *node = (ExprUnaryNode*) n;
            fn(node->right, data);
            break;
        }
        case EXPR_VAR_ASSIGN: {
            ExprVarAssignNode *node = (ExprVarAssignNode*) n;
            fn(node->target, data);
            fn(node->value, data);
            break;
        }
        case EXPR_INC_DEC: {
            ExprIncDecNode *node = (ExprIncDecNode*) n;
            fn(node->target, data);
            break;
        }
    }
}