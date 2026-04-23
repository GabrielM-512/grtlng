#pragma once
#include <stddef.h>

typedef struct {
    char *source;
    size_t fileSize;
} TextFile;

TextFile textfileRead(const char *source);