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
    const char *hint;
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

        if (current.hint != nullptr) {
            fprintf(stderr, "\nHint: %s\n", current.hint);
        }

        if (i + 1 < handler.errors->length) fprintf(stderr, "\n\n\n");
    }

}

static void enqueueError(const char *message, va_list args, const Token token, const char* hint) {
    const u32 length = vsnprintf(handler.testing, 256, message, args) + 1;

    char *target = ArenaAlloc(handler.data, length);

    strcpy(target, handler.testing);

    for (u16 i = 0; i < 256; i++) {
        handler.testing[i] = 0;
    }

    const Error error = {
        .token = token,
        .message = target,
        .hint = hint
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
    fprintf(stderr, "%-*.*s^ Here\n", hatStart, hatStart, "");
}

static void parseErrorAt(Parser *parser, const Token token, const char* hint, const char* message, va_list args) {
    if (parser->panicMode) {
        return;
    }

    parser->panicMode = true;
    parser->hadError = true;

    enqueueError(message, args, token, hint);

    handler.source = parser->source;

}

void parseErrorAtCurrentHint(Parser *parser, const char* hint, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, parser->current, hint, message, args);
    va_end(args);
}

void parseErrorAtCurrent(Parser *parser, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, parser->current, nullptr, message, args);
    va_end(args);
}

void parseErrorHint(Parser *parser, const char* hint, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, parser->previous, hint, message, args);
    va_end(args);
}

void parseError(Parser *parser, const char* message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args, message);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, parser->previous, nullptr, message, args);
    va_end(args);
}

void parseErrorAtToken(Parser *parser, u32 tokenPos, const char *message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args);
    Token errorToken = ArrayListRead(parser->Tokens, tokenPos, Token);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, errorToken, nullptr, message, args);
    parser->panicMode = false;
}

void parseErrorAtTokenHint(Parser *parser, u32 tokenPos, const char *hint, const char *message, ...) {
    va_list args;
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    va_start(args);
    Token errorToken = ArrayListRead(parser->Tokens, tokenPos, Token);
    // ReSharper disable once CppLocalVariableMightNotBeInitialized
    parseErrorAt(parser, errorToken, hint, message, args);
    parser->panicMode = false; // this is only called after we have created an AST and can't get confused about where we are anymore
    // so panic mode would only stop us from sending all error messages
}

void expectedGotInstead(Parser *parser, const char* location, TokenType expected, TokenType got) {
    parseErrorAtCurrent(parser, "Expected %s%s, got %s instead", getTokenSymbol(expected), location, getTokenSymbol(got));
}