#include "parser.h"

#include <stdio.h>

#include "parseUtils.h"
#include "scoping.h"
#include "globalScope.h"
#include "resolving.h"

#include "../error.h"
#include "../debug/debugInfos.h"

void incDecExpr(ExprNode *node, void *e) {
    if (node == nullptr) return;

    Expr *expr = e;

    switch (node->type) {
        // keep recursing
        case EXPR_BINARY:
        case EXPR_UNARY:
        case EXPR_VAR_ASSIGN: // checking for assignability happens later, so we dont concern ourselves with that here
            recurseExpr(node, incDecExpr, expr);
            break;

            // end
        case EXPR_VAR:
        case EXPR_NUMBER:
            break;

            // do stuff
        case EXPR_CALL: { // each argument is a separate expression
            const ExprCallNode *call = (ExprCallNode *) node;
            incDecExpr(call->target, expr);

            for (u32 i = 0; i < call->args.length; i++) {
                Expr *arg = ArrayListRead(&call->args, i, Expr*);
                incDecExpr(arg->expr, arg);
            }
            break;
        }
        case EXPR_INC_DEC: {
            ExprIncDecNode *inc = (ExprIncDecNode*) node;
            ArrayList *time = inc->time ? &expr->prefix : &expr->suffix;
            ArrayListAdd(time, &inc);
            break;
        }
    }
}

void incDec(Parser*, Expr *expr) {
    incDecExpr(expr->expr, expr);
}

void handleIncDec(Parser *parser) {
    reachExpr(parser, incDec);
}

void parserInit(Parser *parser) {
    parser->token = 0;


    ArrayListInit(&parser->program.tree, sizeof(StmtNode*));
    parser->program.data = ArenaNew();

    parser->hadError = false;
    parser->panicMode = false;

    parser->inGlobalPhase = true;

    parser->currentScope = nullptr;
    beginScope(parser);

    advance(parser);
}

ParseResult parseAll(Parser *parser, ArrayList *tokens, const char* source) {

    parser->Tokens = tokens;
    parser->source = source;

    parserInit(parser);

    while (!check(parser, TOKEN_EOF)) {
        StmtNode *declaration = globalDeclaration(parser);
        if (declaration == nullptr) {
            synchronise(parser);
            continue;
        }

        ArrayListAdd(&parser->program.tree, &declaration);
    }

    resolve(parser);
    handleIncDec(parser);


    // call main to finish init segment
    if (!symbolExists(parser, "main")) {
        fprintf(stderr, "Encountered error in program: No main function in program\n");
        parser->hadError = true;
        return parser->program;
    }

    Symbol main = getSymbol(parser, "main");

    if (!IS_FUNC(main)) {
        fprintf(stderr, "Encountered error in program: Symbol \"main\" must be a function");
        parser->hadError = true;
        return parser->program;
    }

    switch (AS_FUNC(main)->returns) {
        case TOKEN_VOID:
        case TOKEN_I16:
        case TOKEN_I32:
        case TOKEN_I64:
        case TOKEN_U16:
        case TOKEN_U32:
        case TOKEN_U64:
            break;
        default:
            fprintf(stderr, "Encountered error in program: Return type of main function must be \"void\" or any type of signed or unsigned integer, was defined as %s instead\n", getTokenSymbol(AS_FUNC(main)->returns));
            parser->hadError = true;
            return parser->program;
    }

    ExprCallNode mainCall;
    mainCall.header.type = EXPR_CALL;

    ExprVarNode *target = ALLOC_NODE(ExprVarNode);

    target->header.type = EXPR_VAR;
    target->name = "main";

    mainCall.target = (ExprNode*) target;
    mainCall.header.token = AS_FUNC(main)->header.token;

    ArrayListInit(&mainCall.args, sizeof(ExprNode*));

    parser->program.main = mainCall;

    return parser->program;
}
