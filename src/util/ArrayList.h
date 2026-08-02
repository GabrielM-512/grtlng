#pragma once
#include <stddef.h>
#include "../global.h"

#define ArrayListRead(list, index, type) (*(type*)ArrayListGet(list, index))

typedef struct {
    u32 capacity, length;
    size_t elementSize;
    void* elements;
} ArrayList;

void ArrayListInit(ArrayList *list, size_t elementSize);

ArrayList *ArrayListNew(size_t elementSize);
void ArrayListFree(ArrayList *list);

void ArrayListAdd(ArrayList *list, const void *element);
void *ArrayListGet(const ArrayList *list, u32 index);
