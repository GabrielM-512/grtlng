#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token scanToken(Lexer* lexer);
void skipWhitespace(Lexer *lexer);

Lexer *LexerNew(const char *source) {
    Lexer *lexer = malloc(sizeof(Lexer));

    // read the file into the lexer struct
    {
        FILE *file = fopen(source, "rb");

        if (file == nullptr) {
            fprintf(stderr, "Problem opening file %s\n", source);
            exit(1);
        }

        fseek(file, 0L, SEEK_END);
        lexer->length = ftell(file) + 1;
        lexer->source = malloc(lexer->length);

        // reset to start of file
        rewind(file);

        //read the file
        fread(lexer->source, sizeof(char), lexer->length, file);
        fclose(file);

        lexer->source[lexer->length] = '\0';
    }

    lexer->base = 0;
    lexer->head = 0;

    lexer->line = 1;

    lexer->data = ArenaAllocNew();

    return lexer;
}

void LexerFree(Lexer* lexer) {
    free(lexer->source);
    free(lexer);

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


bool isAtEnd(const Lexer *lexer) {
    return lexer->source[lexer->head] == '\0';
}

char advance(Lexer *lexer) {
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


Token number(Lexer *lexer) {
    while (isdigit(peek(lexer))) advance(lexer);

    if (peek(lexer) == '.') {
        advance(lexer);
        while (isdigit(peek(lexer))) advance(lexer);
    }

    const double value = strtod(lexer->source + lexer->base, nullptr);
    double *data = ArenaAllocAlloc(lexer->data, sizeof(double));

    *data = value;

    return makeToken(lexer, TOKEN_NUM, data);
}

Token string(Lexer *lexer) {
    while (peek(lexer) != '"' && !isAtEnd(lexer)) {
        advance(lexer);
    }

    if (isAtEnd(lexer)) return errorToken(lexer, "Unterminated string");
    advance(lexer); // eat last '"'

    const u32 beginning = lexer->base + 1; // skip first '"'
    const u32 size = lexer->head - beginning; // Text + 1 byte for \0

    char *data = ArenaAllocAlloc(lexer->data, size);
    memcpy(data, lexer->source + lexer->base + 1, size - 1);

    data[size - 1] = '\0';

    return makeToken(lexer, TOKEN_STRING, data);
}


Token scanToken(Lexer* lexer) {

    skipWhitespace(lexer);
    lexer->base = lexer->head;

    if (isAtEnd(lexer)) return noDataToken(lexer, TOKEN_EOF);

    char c = advance(lexer);

    switch (c) {

        case ';': return noDataToken(lexer, TOKEN_SEMICOLON);
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
        default:
            break;
    }

    if (isdigit(c)) return number(lexer);
    if (c == '"') return string(lexer);

    while (peek(lexer) != ' ' && peek(lexer) != '\n' && !isAtEnd(lexer)) {
        advance(lexer);
    }

    return errorToken(lexer, "Unexpected character");
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
            default: {
                    return;
                }
        }
    }

}