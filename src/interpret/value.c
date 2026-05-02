#include "value.h"

#include <string.h>

uint32_t hashString(const char* key) {
    u32 hash = 2166136261u;
    const u16 length = strlen(key);
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}
