#include "parser.h"

#include <stdio.h>

#include "parseUtils.h"
#include "scoping.h"
#include "globalScope.h"
#include "statements.h"

#include "../error.h"
#include "../util/HashMap.h"

void parseFunction(Parser *parser, FunctionDeclaration declaration) {
    // set parser to beginning of function and parse body as block
    parser->token = declaration.start;
    advance(parser);
    consume(parser, TOKEN_LEFT_BRACE, " after function declaration");

    StmtBlockNode *body = (StmtBlockNode*) blockStmt(parser);

    // add body to Function in HashMap
    StmtFunction function;
    HashMapGet(&parser->program.functions, declaration.name, &function);
    function.body = body;

    HashMapSet(&parser->program.functions, function.name, &function);
}

void parserInit(Parser *parser) {
    parser->token = 0;

    parser->program.tree = ArrayListNew(sizeof(StmtNode*));
    parser->program.data = ArenaNew();
    HashMapInit(&parser->program.functions, sizeof(StmtFunction));

    parser->hadError = false;
    parser->panicMode = false;

    parser->inGlobalPhase = true;

    parser->currentScope = nullptr;
    beginScope(parser);

    advance(parser);
}

ParseResult parseAll(Parser *parser, ArrayList *tokens, const char* source) {
    /*
     * Plan:
     * parse entire source file for declarations
     * put variable declarations at the beginning of the AST (now called init phase)
     * put function declarations into own ArrayList (name, starting token)
     * parse each function
     * put call to main() at the end of init phase AST
     */

    parser->Tokens = tokens;
    parser->source = source;

    parserInit(parser);

    // parse global declarations (functions and variables)
    ArrayList *functions = parseGlobals(parser);

    parser->inGlobalPhase = false;

    // parse function bodies
    for (u32 i = 0; i < functions->length; i++) {
        // pull next function from queue
        FunctionDeclaration declaration = ArrayListRead(functions, i, FunctionDeclaration);

        parseFunction(parser, declaration);

    }

    // call main to finish init segment
    if (!HashMapHas(&parser->program.functions, "main")) {
        fprintf(stderr, "Encountered error in program: No main function in program\n");
        parser->hadError = true;
        return parser->program;
    }

    ExprCallNode *mainCall = ALLOC_NODE(ExprCallNode);
    mainCall->header.type = EXPR_CALL;
    mainCall->target = "main";
    mainCall->args = ArrayListNew(sizeof(ExprNode*));

    StmtExprNode *call = ALLOC_NODE(StmtExprNode);
    call->header.type = STMT_EXPR;
    call->expr = (ExprNode*) mainCall;

    ArrayListAdd(parser->program.tree, &call);

    return parser->program;
}
