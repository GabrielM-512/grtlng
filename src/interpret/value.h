#pragma once

#include "../global.h"
#include "../parser/parser.h"

typedef enum {
    VAL_NUM,
    VAL_FUNC,
} ValueType;

typedef struct {
    ValueType type;
    union {
        f64 num;
        bool boolean;
        StmtFunction *func;
    } as;
} Value;

void printValue(Value val);

#define VALUE_FUNC(value) ((Value) {.type = VAL_FUNC, {.func = value}})
#define VALUE_NUM(value) ((Value) {.type = VAL_NUM, {.num = value}})
#define VALUE_NAN (VALUE_NUM(NAN))
#define VALUE_BOOL(value) (VALUE_NUM(value ? 1 : 0))
#define VALUE_TRUE (VALUE_BOOL(true))
#define VALUE_FALSE (VALUE_BOOL(false))

#define AS_FUNC(value) (value.as.func)
#define AS_NUM(value) (value.as.num)

#define IS_FUNC(value) (value.type == VAL_FUNC)
#define IS_NUM(value) (value.type == VAL_NUM)