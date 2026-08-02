#pragma once

#include "environment.h"
#include "value.h"
#include "../parser/parser.h"

typedef struct {
    Environment *env;
    Environment *global;
    bool returning;
    Value returnValue;
} Interpreter;

extern Interpreter interpreter;

void interpretBlock(StmtBlockNode *node);

i32 interpretProgram(ParseResult program);
