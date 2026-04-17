#pragma once
#include <stdint.h>

#define INTERN_ERROR_LOCATION(file, line) (fprintf(stderr, "[ERROR] %s on line%d\n", file, line))

#define DEBUG_ALL

#ifdef DEBUG_ALL
 #define DEBUG_PRINT_TOKENS
#endif


typedef uint8_t u8;
typedef int8_t i8;

typedef uint16_t u16;
typedef int16_t i16;

typedef uint32_t u32;
typedef int32_t i32;