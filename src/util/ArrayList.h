#pragma once
#include <stddef.h>
#include "../global.h"

#define ArrayListRead(list, index, type) (*(type*)ArrayListGet(list, index))

typedef struct {
    u32 capacity, size;
    size_t elementSize;
    void* elements;
} ArrayList;

ArrayList *ArrayListNew(size_t elementSize);

void ArrayListFree(ArrayList *list);

void ArrayListAdd(ArrayList *list, const void *element);
void *ArrayListGet(const ArrayList *list, u32 index);
