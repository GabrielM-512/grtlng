#pragma once

#include "lexer.h"
#include "parser/parser.h"

void parseErrorAtCurrent(Parser *parser, const char* message, ...);
void parseError(Parser *parser, const char* message, ...);

void parseErrorAtCurrentHint(Parser *parser, const char* hint, const char* message, ...);
void parseErrorHint(Parser *parser, const char* hint, const char* message, ...);

void parseErrorAtToken(Parser *parser, u32 tokenPos, const char *message, ...);
void parseErrorAtTokenHint(Parser *parser, u32 tokenPos, const char *hint, const char *message, ...);

void expectedGotInstead(Parser *parser, const char* location, TokenType expected, TokenType got);

void errorSetup();
void printErrors();