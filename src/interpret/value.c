#include "value.h"

#include <stdio.h>

void printValue(Value val) {
    switch (val.type) {
        case VAL_NUM:
            printf("%f", AS_NUM(val));
            break;
        case VAL_FUNC: {
            StmtFunction *function = AS_FUNC(val);
            printf("<fn \"%s\" at %p>", function->name, function);
            break;
        }

        default:
    }
    putchar('\n');
}
