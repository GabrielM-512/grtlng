#include "parser.h"

#include <stdio.h>

#include "parseUtils.h"
#include "scoping.h"
#include "globalScope.h"

#include "../error.h"
#include "../debug/debugInfos.h"
#include "../util/HashMap.h"

void parserInit(Parser *parser) {
    parser->token = 0;

    parser->program.tree = ArrayListNew(sizeof(StmtNode*));
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

        switch (declaration->type) {
            case STMT_FUN_DEC: {
                StmtFunction *func = (StmtFunction*) declaration;
                createVar(parser, func->name, FUNC_SYMBOL(func));
            }
            case STMT_VAR_DEC:
                break;
            default:
                // unreachable
        }

        ArrayListAdd(parser->program.tree, &declaration);
    }


    // call main to finish init segment
    if (!varExists(parser, "main")) {
        fprintf(stderr, "Encountered error in program: No main function in program\n");
        parser->hadError = true;
        return parser->program;
    }

    Symbol main = getVar(parser, "main");

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

    mainCall.args = ArrayListNew(sizeof(ExprNode*));

    parser->program.main = mainCall;

    return parser->program;
}
