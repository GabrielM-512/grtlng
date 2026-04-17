#pragma once
#include "global.h"

#include "util/ArrayList.h"
#include "util/ArenaAlloc.h"

typedef enum {
    TOKEN_EOF, // End of source file
    TOKEN_NUM, // Any number literal
    TOKEN_STRING, // Any String

    TOKEN_SEMICOLON,

    TOKEN_STAR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_SLASH,

    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_STAR_EQUALS,
    TOKEN_SLASH_EQUALS,

    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,

    TOKEN_BANG, // '!'

    TOKEN_EQUALS,
    TOKEN_EQUALS_EQUALS,
    TOKEN_MORE_EQUALS,
    TOKEN_LESS_EQUALS,
    TOKEN_BANG_EQUALS,

    TOKEN_ERROR,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    u16 line;
    void *data; //May hold any data the Token needs from the source string (e.g. Literals, Variable/Function names, etc)
} Token;

/*
 * The construct for creating Tokens out of the source string.
 * The Lexer owns the source string as soon as it is passed. It will free the source string upon being destroyed.
 */
typedef struct {
    char *source;
    u32 length;
    u32 base, head;
    u16 line;
    ArenaAlloc *data;
} Lexer;


Lexer *LexerNew(const char *source);

void LexerFree(Lexer* lexer);

ArrayList *scanAll(Lexer* lexer);