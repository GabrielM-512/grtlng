#include "resolving.h"

#include "scoping.h"
#include "../error.h"

// todo: make proper error reporting possible here -> include pointer to relevant token with each stmt/expression?
// todo: check the arity on call expressions

typedef void (*StmtResolveFn)(Parser*, StmtNode*);
typedef void (*ExprResolveFn)(Parser*, ExprNode*);

void resolveExpr(Parser *parser, ExprNode *node);
void resolveStmt(Parser *parser, StmtNode *node);


void globalVarDec(Parser *parser, StmtNode *node) {

    Symbol symbol;
    char *name;

    switch (node->type) {
        case STMT_VAR_DEC:
            StmtVarDeclNode *var = (StmtVarDeclNode*) node;
            name = var->name;
            symbol = VAR_SYMBOL(var);
            break;
        case STMT_FUN_DEC:
            StmtFunction *func = (StmtFunction*) node;
            name = func->name;
            symbol = FUNC_SYMBOL(func);
            break;
        default:
            name = nullptr;
            symbol = (Symbol) {
                .type = 0,
                .initialised = false,
                .as = {nullptr}
            };
    }

    if (symbolExists(parser, name)) {
        parseErrorAtToken(parser, node->token, "Redeclared global Symbol \"%s\"", name);
    } else {
        createSymbol(parser, name, symbol);
    }


    if (node->type == STMT_VAR_DEC) {
        StmtVarDeclNode *var = (StmtVarDeclNode*) node;
        if (var->value != nullptr) resolveExpr(parser, var->value->expr);
        activateSymbol(parser, name);
    }

}

void resolve(Parser *parser) {
    StmtNode *funcs [parser->program.tree.length];
    u32 funcCount = 0;
    for (u32 i = 0; i < parser->program.tree.length; i++) {
        StmtNode *node = ArrayListRead(&parser->program.tree, i, StmtNode*);

        globalVarDec(parser, node);

        if (node->type == STMT_FUN_DEC) {
            funcs[funcCount++] = node;
        }
    }

    for (u32 i = 0; i < funcCount; i++) {
        StmtNode *function = funcs[i];
        resolveStmt(parser, function);
    }
}

/*
     SSSS   TTTTT     A     TTTTT   EEEEE   M   M   EEEEE   N   N   TTTTT    SSSS
    S         T      A A      T     E       MM MM   E       NN  N     T     S
     SSS      T      AAA      T     EEEEE   M M M   EEEEE   N N N     T      SSS
        S     T     A   A     T     E       M   M   E       N  NN     T         S
    SSSS      T     A   A     T     EEEEE   M   M   EEEEE   N   N     T     SSSS
 */

void varDeclaration(Parser *parser, StmtNode *n) {
    StmtVarDeclNode *node = (StmtVarDeclNode*) n;

    if (symbolInCurrentScope(parser, node->name)) {
        parseErrorAtToken(parser, node->header.token, "Redeclared Variable \"%s\" in same scope", node->name);
    } else {
        createSymbol(parser, node->name, VAR_SYMBOL(node));
    }

    if (node->value != nullptr) resolveExpr(parser, node->value->expr);

    activateSymbol(parser, node->name);
}

void func(Parser *parser, StmtNode *n) {
    StmtFunction *node = (StmtFunction*) n;

    beginScope(parser);

    for (u32 i = 0; i < node->parameters.length; i++) {
        StmtVarDeclNode *parameter = ArrayListRead(&node->parameters, i, StmtVarDeclNode*);

        if (symbolInCurrentScope(parser, parameter->name)) {
            parseErrorAtToken(parser, parameter->header.token, "Redeclared parameter \"%s\" in function \"%s\"", parameter->name, node->name);
            continue;
        }

        createSymbol(parser, parameter->name, VAR_SYMBOL(parameter));
        activateSymbol(parser, parameter->name);
    }

    parser->inGlobalPhase = false;

    resolveStmt(parser, (StmtNode*) node->body);

    parser->inGlobalPhase = true;

    endScope(parser);
}

void expr(Parser *parser, StmtNode *n) {
    StmtExprNode *node = (StmtExprNode*) n;
    resolveExpr(parser, node->expr->expr);
}

void block(Parser *parser, StmtNode *n) {
    StmtBlockNode *node = (StmtBlockNode*) n;

    beginScope(parser);

    for (u32 i = 0; i < node->content.length; i++) {
        StmtNode *stmt = ArrayListRead(&node->content, i, StmtNode*);
        resolveStmt(parser, stmt);
    }

    endScope(parser);
}

void return_(Parser *parser, StmtNode *n) {
    StmtReturnNode *node = (StmtReturnNode*) n;
    if (node->value != nullptr) resolveExpr(parser, node->value->expr);
}

void if_(Parser *parser, StmtNode *n) {
    StmtIfNode *node = (StmtIfNode*) n;

    resolveExpr(parser, node->condition->expr);
    resolveStmt(parser, node->thenBranch);
    if (node->elseBranch != nullptr) resolveStmt(parser, node->elseBranch);
}

void print(Parser *parser, StmtNode *n) {
    StmtPrintNode *node = (StmtPrintNode*) n;
    resolveExpr(parser, node->value->expr);
}

void while_(Parser *parser, StmtNode *n) {
    StmtWhileNode *node = (StmtWhileNode*) n;
    resolveExpr(parser, node->condition->expr);
    resolveStmt(parser, node->body);
}

/*
    EEEEE   X   X   PPPP    RRRR    EEEEE    SSSS    SSSS    III     OOO    N   N    SSSS
    E        X X    P   P   R   R   E       S       S        III    O   O   NN  N   S
    EEEEE     X     PPPP    RRRR    EEEEE    SSS     SSS     III    O   O   N N N    SSS
    E        X X    P       R  R    E           S       S    III    O   O   N  NN       S
    EEEEE   X   X   P       R   R   EEEEE   SSSS    SSSS     III     OOO    N   N   SSSS
 */

void binary(Parser *parser, ExprNode *n) {
    ExprBinaryNode *node = (ExprBinaryNode*) n;
    resolveExpr(parser, node->left);
    resolveExpr(parser, node->right);
}

void unary(Parser *parser, ExprNode *n) {
    ExprUnaryNode *node = (ExprUnaryNode*) n;
    resolveExpr(parser, node->right);
}

void var(Parser *parser, ExprNode *n) {
    ExprVarNode *node = (ExprVarNode*) n;
    if (!symbolExists(parser, node->name)) {
        parseErrorAtToken(parser, node->header.token, "Couldn't resolve symbol \"%s\"", node->name);
    }
}

void varAssign(Parser *parser, ExprNode *n) {
    ExprVarAssignNode *node = (ExprVarAssignNode*) n;
    resolveExpr(parser, node->target);
    resolveExpr(parser, node->value);
}

static void call(Parser *parser, ExprNode *n) {
    ExprCallNode *node = (ExprCallNode*) n;
    if (parser->inGlobalPhase) {
        parseErrorAtToken(parser, node->header.token, "Tried using a function call to initialise a global variable");
        return;
    }
    resolveExpr(parser, node->target);

    for (u32 i = 0; i < node->args.length; i++) {
        Expr *arg = ArrayListRead(&node->args, i, Expr*);
        resolveExpr(parser, arg->expr);
    }
}

static StmtResolveFn stmtFunctions[] = {
    [STMT_VAR_DEC] = varDeclaration,
    [STMT_FUN_DEC] = func,
    [STMT_EXPR] = expr,
    [STMT_BLOCK] = block,
    [STMT_RETURN] = return_,
    [STMT_IF] = if_,
    [STMT_PRINT] = print,
    [STMT_WHILE] = while_
};


static ExprResolveFn exprFunctions[] = {
    [EXPR_BINARY] = binary,
    [EXPR_UNARY] = unary,
    [EXPR_NUMBER] = nullptr, // the -Werror flag would mean that leaving the number function in would generate errors as there is nothing to check (yet)
    [EXPR_VAR] = var,
    [EXPR_CALL] = call,
    [EXPR_VAR_ASSIGN] = varAssign
};

void resolveStmt(Parser *parser, StmtNode *node) {
    stmtFunctions[node->type](parser, node);
}

void resolveExpr(Parser *parser, ExprNode *node) {
    // doing this is safe as every return of a nullptr as a node only happens if an error has already been triggered, meaning none of this gets executed
    if (node == nullptr) return;
    ExprResolveFn fn = exprFunctions[node->type];
    if (fn != nullptr) exprFunctions[node->type](parser, node);
}