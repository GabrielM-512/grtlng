#include "textfile.h"

#include <stdio.h>
#include <stdlib.h>


// reads in a file as an ascii-file and puts it into a char[] and the length in a size_t.
TextFile textfileRead(const char *source) {
    TextFile file;

    FILE *sourceFile = fopen(source, "rb");

    if (sourceFile == nullptr) {
        fprintf(stderr, "Problem opening file %s\n", source);
        exit(1);
    }

    fseek(sourceFile, 0L, SEEK_END);
    file.fileSize = ftell(sourceFile) + 1;
    file.source = malloc(file.fileSize);

    // reset to start of file
    rewind(sourceFile);

    //read the file
    fread(file.source, sizeof(char), file.fileSize, sourceFile);
    fclose(sourceFile);

    file.source[file.fileSize - 1] = '\0'; // THE NULL TERMINATOR PLACEMENT WILL KILL ME SOME DAY

    /*
     * FOR CONTEXT:
     * This is the second time where this null terminator byte was placed one byte too late
     * Commit 6c931ed2 already fixed this, but apparently merging some other branch reverted that
     * So fuck Git for that
     */

    return file;
}
