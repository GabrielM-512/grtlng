#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/file-io/textfile.h"

Token scanToken(Lexer* lexer);
void skipWhitespace(Lexer *lexer);

// externally callable functions

void lexerInit(Lexer* lexer, const char *source, ArenaAllocator *tokenData) {

    lexer->source = textfileRead(source).source;

    lexer->base = 0;
    lexer->head = 0;

    lexer->line = 1;

    lexer->data = tokenData;

}


ArrayList *scanAll(Lexer* lexer) {
    ArrayList *tokens = ArrayListNew(sizeof(Token));

    Token token = {TOKEN_UNKNOWN, 1, nullptr};
    skipWhitespace(lexer);

    while (token.type != TOKEN_EOF) {
        token = scanToken(lexer);
        ArrayListAdd(tokens, &token);
    }

    return tokens;
}

// utils

bool isAlpha(const char c) {
    return ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z');
}

bool isNum(const char c) {
    return '0' <= c && c <= '9';
}

bool isAlNum(const char c) {
    return isAlpha(c) || isNum(c);
}


bool isAtEnd(const Lexer *lexer) {
    return lexer->source[lexer->head] == '\0';
}

static char advance(Lexer *lexer) {
    lexer->head++;
    return lexer->source[lexer->head - 1];
}

char peek(const Lexer *lexer) {
    return lexer->source[lexer->head];
}

char peekNext(const Lexer *lexer) {
    if (isAtEnd(lexer)) return '\0';
    return lexer->source[lexer->head + 1];
}


Token makeToken(const Lexer *lexer, const TokenType type, void* data) {
    return (Token) {type, lexer->line, data};
}

Token noDataToken(const Lexer *lexer, const TokenType type) {
    return makeToken(lexer, type, nullptr);
}

Token errorToken(const Lexer *lexer, char* message) {
    return (Token) {TOKEN_ERROR, lexer->line, message};
}

// internal functions


Token number(Lexer *lexer) {
    while (isNum(peek(lexer))) advance(lexer);

    if (peek(lexer) == '.') {
        advance(lexer);
        while (isNum(peek(lexer))) advance(lexer);
    }

    const double value = strtod(lexer->source + lexer->base, nullptr);
    double *data = ArenaAlloc(lexer->data, sizeof(double));

    *data = value;

    return makeToken(lexer, TOKEN_NUM, data);
}

Token string(Lexer *lexer) {
    const u16 start = lexer->line;
    while (peek(lexer) != '"' && !isAtEnd(lexer)) {
        if (advance(lexer) == '\n') lexer->line++;
    }

    if (isAtEnd(lexer)) return (Token) {TOKEN_ERROR, start, "Unterminated string."};
    advance(lexer); // eat last '"'

    const u32 beginning = lexer->base + 1; // skip first '"'
    const u32 size = lexer->head - beginning; // Text + 1 byte for \0

    if (size > lexer->data->capacity) {
        return (Token) {TOKEN_ERROR, start, "Strings may not exceed 4096 characters in size."};
    }


    char *data = ArenaAlloc(lexer->data, size);
    memcpy(data, lexer->source + lexer->base + 1, size - 1);

    data[size - 1] = '\0';

    return (Token) {TOKEN_STRING, start, data};
}

Token identifier(Lexer *lexer) {

    while (true) {
        const char c = peek(lexer);
        if (!(isAlNum(c) || c == '_')) break;
        advance(lexer);
    }

    // check for keywords here

    const size_t size = lexer->head - lexer->base + 1;

    if (size > lexer->data->capacity) {
        return errorToken(lexer, "Identifiers may not exceed 4096 characters in size.");
    }

    char *data = ArenaAlloc(lexer->data, size);
    memcpy(data, lexer->source + lexer->base, size);
    data[size - 1] = '\0';
    return makeToken(lexer, TOKEN_IDENTIFIER, data);

}


Token scanToken(Lexer* lexer) {

    skipWhitespace(lexer);
    lexer->base = lexer->head;

    if (isAtEnd(lexer)) return noDataToken(lexer, TOKEN_EOF);

    char c = advance(lexer);

    switch (c) {

        // single-character tokens
        case ';': return noDataToken(lexer, TOKEN_SEMICOLON);
        case '(': return noDataToken(lexer, TOKEN_LEFT_PAREN);
        case ')': return noDataToken(lexer, TOKEN_RIGHT_PAREN);
        case '{': return noDataToken(lexer, TOKEN_LEFT_BRACE);
        case '}': return noDataToken(lexer, TOKEN_RIGHT_BRACE);
        case '[': return noDataToken(lexer, TOKEN_LEFT_BRACKET);
        case ']': return noDataToken(lexer, TOKEN_RIGHT_BRACKET);

        case ',': return noDataToken(lexer, TOKEN_COMMA);
        case '.': return noDataToken(lexer, TOKEN_PERIOD);

        case '~': return noDataToken(lexer, TOKEN_TILDE);

        // possibly two-character-tokens
        case '+':
            switch (peek(lexer)) {
                case '=': advance(lexer); return noDataToken(lexer, TOKEN_PLUS_EQUALS);
                case '+': advance(lexer); return noDataToken(lexer, TOKEN_PLUS_PLUS);
                default:
                    return noDataToken(lexer, TOKEN_PLUS);
            }
        case '-':
            switch (peek(lexer)) {
            case '=': advance(lexer); return noDataToken(lexer, TOKEN_MINUS_EQUALS);
            case '-': advance(lexer); return noDataToken(lexer, TOKEN_MINUS_MINUS);
            default:
                    return noDataToken(lexer, TOKEN_MINUS);
            }
        case '*':
            if (peek(lexer) == '=') {
                advance(lexer);
                return noDataToken(lexer, TOKEN_STAR_EQUALS);
            }
            return noDataToken(lexer, TOKEN_STAR);
        case '/':
            if (peek(lexer) == '=') {
                advance(lexer);
                return noDataToken(lexer, TOKEN_SLASH_EQUALS);
            }
            return noDataToken(lexer, TOKEN_SLASH);

        case '=':
            if (peek(lexer) == '=') {
                advance(lexer);
                return noDataToken(lexer, TOKEN_EQUALS_EQUALS);
            }
            return noDataToken(lexer, TOKEN_EQUALS);
        case '!':
            if (peek(lexer) == '=') {
                advance(lexer);
                return noDataToken(lexer, TOKEN_BANG_EQUALS);
            }
            return noDataToken(lexer, TOKEN_BANG);
        case '>':
            if (peek(lexer) == '=') {
                advance(lexer);
                return noDataToken(lexer, TOKEN_MORE_EQUALS);
            }
            return noDataToken(lexer, TOKEN_MORE);
        case '<':
            if (peek(lexer) == '=') {
                advance(lexer);
                return noDataToken(lexer, TOKEN_LESS_EQUALS);
            }
            return noDataToken(lexer, TOKEN_LESS);

        case '|':
            switch (peek(lexer)) {
                case '=': advance(lexer); return noDataToken(lexer, TOKEN_PIPE_EQUALS);
                case '|': advance(lexer); return noDataToken(lexer, TOKEN_PIPE_PIPE);

                default: return noDataToken(lexer, TOKEN_PIPE);
            }

        case '&':
            switch (peek(lexer)) {
                case '=': advance(lexer); return noDataToken(lexer, TOKEN_AMP_EQUALS);
                case '&': advance(lexer); return noDataToken(lexer, TOKEN_AMP_AMP);

                default: return noDataToken(lexer, TOKEN_AMP);
            }

        default:
            break;
    }

    // multi-character tokens

    if (isNum(c)) return number(lexer);
    if (c == '"') return string(lexer);

    // check for keywords here

    if (isAlpha(c) || c == '_') return identifier(lexer);

    return errorToken(lexer, "Unexpected character.");
}



/**
 * @return Whether or not this was a comment (true = comment, false = no comment)
 */
bool comment(Lexer *lexer) {
    const char next = peekNext(lexer);

    switch (next) {
        case '/': //line comment
            while (peek(lexer) != '\n') advance(lexer);
            return true;
        case '*': //block comment
            while (peek(lexer) != '*' || peekNext(lexer) != '/') {
                if (peek(lexer) == '\n') lexer->line++;
                if (isAtEnd(lexer)) return true;
                advance(lexer);
            }
            advance(lexer); // consume trailing */
            advance(lexer);
            return true;
        default:
            return false;
    }

}

void skipWhitespace(Lexer *lexer) {
    while (true) {
        const char c = peek(lexer);

        switch (c) {
            case ' ':
            case '\t':
            case '\r': {
                advance(lexer);
                break;
            }
            case '\n': {
                lexer->line++;
                advance(lexer);
                break;
            }
            case '/':
                if (!comment(lexer)) return;
                break;
            default: return;

        }
    }

}