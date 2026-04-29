#include "debugInfos.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../util/file-io/textfile.h"

ArenaAllocator *text = nullptr;

char* lookup[TOKEN_UNKNOWN + 1];
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
    for (int i = 0; i <= TOKEN_UNKNOWN; i++) {
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