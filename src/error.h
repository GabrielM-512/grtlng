#pragma once

#include "lexer.h"
#include "parser/parser.h"

void parseErrorAtCurrent(Parser *parser, const char* message, ...);
void parseError(Parser *parser, const char* message, ...);

void expectedGotInstead(Parser *parser, const char* location, TokenType expected, TokenType got);

void errorSetup();
void printErrors();