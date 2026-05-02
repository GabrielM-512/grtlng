#pragma once
#include "global.h"

#include "util/ArrayList.h"
#include "util/ArenaAllocator.h"

typedef enum {
    TOKEN_EOF, // End of source file
    TOKEN_ERROR,

    TOKEN_NUM, // Any number literal
    TOKEN_STRING, // Any String

    TOKEN_SEMICOLON,

    TOKEN_LEFT_PAREN, // ()
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, // {}
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, // []
    TOKEN_RIGHT_BRACKET,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_STAR_EQUALS,
    TOKEN_SLASH_EQUALS,

    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,

    TOKEN_AMP, // '&'
    TOKEN_PIPE, // '|'
    TOKEN_TILDE, // '~'

    TOKEN_AMP_AMP,
    TOKEN_PIPE_PIPE,

    TOKEN_AMP_EQUALS,
    TOKEN_PIPE_EQUALS,

    TOKEN_BANG, // '!'
    TOKEN_DOT,
    TOKEN_COMMA,

    TOKEN_MORE,
    TOKEN_LESS,

    TOKEN_EQUALS,
    TOKEN_EQUALS_EQUALS,
    TOKEN_MORE_EQUALS,
    TOKEN_LESS_EQUALS,
    TOKEN_BANG_EQUALS,

    TOKEN_IDENTIFIER,

    // type tokens
    TOKEN_I16,

    TOKEN_LAST // a marker token to always be the last token
} TokenType;

typedef struct {
    TokenType type;
    u16 line;
    void *data; //May hold any data the Token needs from the source string (e.g. Literals, Variable/Function names, etc)
} Token;

/*
 * The construct for creating Tokens out of the source string.
 */
typedef struct {
    char *source;
    u32 base, head;
    u16 line;
    ArenaAllocator *data;
} Lexer;


void lexerInit(Lexer* lexer, const char *source, ArenaAllocator *tokenData);

ArrayList *scanAll(Lexer* lexer);