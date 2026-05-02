#include "debugInfos.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../util/file-io/textfile.h"

ArenaAllocator *text = nullptr;

char* lookup[TOKEN_LAST];
bool bHasFailed = false;

void populate_table() {
    if (bHasFailed) return;

    TextFile file = textfileRead("/home/gabriel/CLionProjects/language/src/lexer.h");

    // look for the enum
    u32 start;
    for (u32 i = 0;; i++) {
        if (i + 9 > file.fileSize) {
            fprintf(stderr, "[DEBUG ERROR] %s: Couldnt locate enum start\n", __FILE__);
            bHasFailed = true;
            free(file.source);
            return;
        }

        const char *target = "TOKEN_EOF";
        if (memcmp(target, &file.source[i], 9) == 0) {
            start = i;
            break;
        }
    }

    text = ArenaNew();

    // transfer tokens
    for (int i = 0; i < TOKEN_LAST; i++) {
        int tokenlength = 0;

        // search for token end
        char c = file.source[start];
        while (c != ',') {
            tokenlength++;
            c = file.source[start + tokenlength];
        }
        tokenlength++;

        // store into the lookup table

        char *textdata = ArenaAlloc(text, tokenlength);

        memcpy(textdata, &file.source[start], tokenlength);
        textdata[tokenlength - 1] = '\0';

        start += tokenlength;

        lookup[i] = textdata;


        // skip whitespace
        c = file.source[start];
        while (c != 'T') {
            start++;
            c = file.source[start];
        }

    }

    free(file.source);
}

bool hasFailed() {
    return bHasFailed;
}

char *getTokenName(TokenType type) {
    if (text == nullptr) populate_table();
    if (bHasFailed) return "TOKEN NAME LOOKUP FAILED";
    return lookup[type];
}

char *getTokenType(TokenType type) {
    if (text == nullptr) populate_table();
    if (bHasFailed) return "TOKEN NAME LOOKUP FAILED";
    return lookup[type] + 6; // cut off "TOKEN_"
}

char *getTokenSymbol(TokenType type) {
    char* table[] = {
        [TOKEN_EOF] = "[End of file]",
        [TOKEN_ERROR] = "an error",
        [TOKEN_NUM] = "a number",
        [TOKEN_STRING] = "a string",
        [TOKEN_SEMICOLON] = "';'",
        [TOKEN_LEFT_PAREN] = "'('",
        [TOKEN_RIGHT_PAREN] = "')'",
        [TOKEN_LEFT_BRACE] = "'{'",
        [TOKEN_RIGHT_BRACE] = "'}'",
        [TOKEN_LEFT_BRACKET] = "'['",
        [TOKEN_RIGHT_BRACKET] = "']'",
        [TOKEN_PLUS] = "'+'",
        [TOKEN_MINUS] = "'-'",
        [TOKEN_STAR] = "'*'",
        [TOKEN_SLASH] = "'/'",
        [TOKEN_PLUS_EQUALS] = "'+='",
        [TOKEN_MINUS_EQUALS] = "'-='",
        [TOKEN_STAR_EQUALS] = "'*='",
        [TOKEN_SLASH_EQUALS] = "'/='",
        [TOKEN_PLUS_PLUS] = "'++'",
        [TOKEN_MINUS_MINUS] = "'--'",
        [TOKEN_AMP] = "'&'",
        [TOKEN_PIPE] = "'|'",
        [TOKEN_TILDE] = "'~'",
        [TOKEN_AMP_AMP] = "'&&'",
        [TOKEN_PIPE_PIPE] = "'||'",
        [TOKEN_AMP_EQUALS] = "'&='",
        [TOKEN_BANG] = "'!'",
        [TOKEN_DOT] = "'.'",
        [TOKEN_COMMA] = "','",
        [TOKEN_MORE] = "'>'",
        [TOKEN_LESS] = "'<'",
        [TOKEN_EQUALS] = "'='",
        [TOKEN_EQUALS_EQUALS] = "'=='",
        [TOKEN_MORE_EQUALS] = "'>='",
        [TOKEN_LESS_EQUALS] = "'<='",
        [TOKEN_BANG_EQUALS] = "'!='",
        [TOKEN_IDENTIFIER] = "an identifier",
        [TOKEN_I16] = "'i16'"
    };

    return table[type];
}