#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "debug/debugInfos.h"

void printErrorLine(Parser* parser, const Token *token) {
    u32 start = token->position;
    while (start > 0 && parser->source[start - 1] != '\n') start--;

    u32 end = token->position;
    for (u8 i = 20; i > 0; i--) {
        end++;
        if (parser->source[end] == '\n' || parser->source[end] == '\0') break;
    }

    const u32 range = end - start;

    char lineString[6];
    sprintf(lineString, "%d", token->line);

    fprintf(stderr, "[%s]   ", lineString);
    fprintf(stderr, "%.*s", range, &parser->source[start]);

    if (token->position + 20 == end) fprintf(stderr, "...");

    fprintf(stderr, "\n");

    const u16 hatStart = token->position - start + 5 + strlen(lineString);
    fprintf(stderr, "%-*.*s^ Here\n\n\n", hatStart, hatStart, "");
}

void parseErrorAt(Parser *parser, const Token* token, const char* message, va_list args) {
    if (parser->panicMode) {
        return;
    }

    parser->panicMode = true;
    parser->hadError = true;

    fprintf(stderr, "Encountered error on line %d: ", token->line);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");


    // print offending line
    printErrorLine(parser, token);

}

void parseErrorAtCurrent(Parser *parser, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, &parser->current, message, args);
    va_end(args);
}

void parseError(Parser *parser, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, &parser->previous, message, args);
    va_end(args);
}

void expectedGotInstead(Parser *parser, const char* location, TokenType expected, TokenType got) {
    parseErrorAtCurrent(parser, "Expected %s%s, got %s instead", getTokenSymbol(expected), location, getTokenSymbol(got));
}