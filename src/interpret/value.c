#include "value.h"

#include <stdio.h>

void printValue(Value val) {
    switch (val.type) {
        case VAL_NUM:
            printf("%f", AS_NUM(val));
            break;
        case VAL_FUNC:
            printf("<fn %s at %p>", AS_FUNC(val)->name, AS_FUNC(val));
            break;
        default:
    }
    putchar('\n');
}
