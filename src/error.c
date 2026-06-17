#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser/parser.h"
#include "lexer.h"
#include "debug/debugInfos.h"

#include "util/ArrayList.h"

typedef struct {
    ArrayList *errors;
    ArenaAllocator *data;
    char *testing;
    const char *source;
} ErrorHandler;

typedef struct {
    Token token;
    const char *message;
} Error;

ErrorHandler handler;

void printErrorLine(const char *source, const Token *token);

void errorSetup() {
    handler.errors = ArrayListNew(sizeof(Error));
    handler.data = ArenaNew();
    handler.testing = malloc(256);
}

void printErrors() {
    Error errors[handler.errors->length];

    for (u32 i = 0; i < handler.errors->length; i++) {
        errors[i] = ArrayListRead(handler.errors, i, Error);
    }

    for (u32 i = 0; i < handler.errors->length; i++) {
        u16 line = errors[i].token.line;
        for (u32 j = i; j < handler.errors->length; j++) {
            if (errors[j].token.line < line) {
                Error temp = errors[j];
                errors[j] = errors[i];
                errors[i] = temp;
            }
        }
    }

    for (u32 i = 0; i < handler.errors->length; i++) {

        Error current = errors[i];

        fprintf(stderr, "Encountered error on line %d: %s\n", current.token.line, current.message);

        // print offending line
        printErrorLine(handler.source, &current.token);
    }

}

void enqueueError(const char *message, va_list args, const Token token) {
    u32 length = vsnprintf(handler.testing, 256, message, args) + 1;

    char *target = ArenaAlloc(handler.data, length);

    strcpy(target, handler.testing);

    for (u16 i = 0; i < 256; i++) {
        handler.testing[i] = 0;
    }

    Error error = {
        token,
        target
    };

    ArrayListAdd(handler.errors, &error);
}

void printErrorLine(const char *source, const Token *token) {
    u32 start = token->position;
    while (start > 0 && source[start - 1] != '\n') start--;

    u32 end = token->position;
    for (u8 i = 20; i > 0; i--) {
        end++;
        if (source[end] == '\n' || source[end] == '\0') break;
    }

    const u32 range = end - start;

    char lineString[6];
    sprintf(lineString, "%d", token->line);

    fprintf(stderr, "[%s]   ", lineString);
    fprintf(stderr, "%.*s", range, &source[start]);

    if (token->position + 20 == end) fprintf(stderr, "...");

    fprintf(stderr, "\n");

    const u16 hatStart = token->position - start + 5 + strlen(lineString);
    fprintf(stderr, "%-*.*s^ Here\n\n\n", hatStart, hatStart, "");
}

void parseErrorAt(Parser *parser, const Token token, const char* message, va_list args) {
    if (parser->panicMode) {
        return;
    }

    parser->panicMode = true;
    parser->hadError = true;

    enqueueError(message, args, token);

    handler.source = parser->source;

}

void parseErrorAtCurrent(Parser *parser, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, parser->current, message, args);
    va_end(args);
}

void parseError(Parser *parser, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, parser->previous, message, args);
    va_end(args);
}

void expectedGotInstead(Parser *parser, const char* location, TokenType expected, TokenType got) {
    parseErrorAtCurrent(parser, "Expected %s%s, got %s instead", getTokenSymbol(expected), location, getTokenSymbol(got));
}