#pragma once

#include "../lexer.h"

char *getTokenName(TokenType type);
char *getTokenType(TokenType type);
char *getTokenSymbol(TokenType type);

extern bool hasFailed;