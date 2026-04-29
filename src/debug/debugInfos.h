#pragma once

#include "../lexer.h"

char *getTokenName(TokenType type);
char *getTokenType(TokenType type);

bool hasFailed();