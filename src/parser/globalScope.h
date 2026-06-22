#pragma once

#include "../util/ArrayList.h"

#include "parser.h"

ArrayList *parseGlobals(Parser *parser);

// used for enqueuing function declarations
typedef struct {
    char *name;
    u32 start;
    ArrayList *parameters;
} FunctionDeclaration;
