#pragma once

#include "../global.h"

typedef enum {
    VALUE_i16,
} ValueTypes;

typedef struct {
    ValueTypes type;
    union {
        i16 i16;
    } value;
} Value;

uint32_t hashString(const char* key);