#pragma once

#include "parser.h"

#define ALLOC_NODE(type) (ArenaAlloc(parser->program.data, sizeof(type)))

void advance(Parser* parser);

bool consume(Parser *parser, TokenType type, const char *message);
bool match(Parser *parser, TokenType type);
bool check (const Parser *parser, TokenType type);

bool isTypeIdent(Parser *parser);
bool matchTypeIdent(Parser *parser);

void synchronise(Parser *parser);


// used for enqueuing function declarations
typedef struct {
    char *name;
    u32 start;
} FunctionDeclaration;

typedef struct {
    TokenType type;
} Variable;