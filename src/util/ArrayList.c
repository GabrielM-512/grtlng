#include "ArrayList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(size, pointer, newCount) (realloc(pointer, size * newCount))

ArrayList *ArrayListNew(const size_t elementSize) {
    ArrayList *list = malloc(sizeof(ArrayList));

    list->capacity = 0;
    list->size = 0;

    list->elementSize = elementSize;

    list->elements = nullptr;

    return list;
}

void ArrayListFree(ArrayList *list) {
    free(list->elements);
    free(list);
}

void ArrayListAdd(ArrayList *list, const void *element) {
    if (list->size + 1 > list->capacity) {
        list->capacity = GROW_CAPACITY(list->capacity);
        list->elements = GROW_ARRAY(list->elementSize, list->elements, list->capacity);

        if (list->elements == nullptr) {
            INTERN_ERROR_LOCATION(__FILE__, __LINE__);
            fprintf(stderr, "Couldnt realloc ArrayList\n");
            exit(1);
        }
    }


    void *dest = list->elements + list->elementSize * list->size;
    list->size++;

    memcpy(dest, element, list->elementSize);

}
void *ArrayListGet(const ArrayList *list, const u32 index) {
    if (index >= list->size) {
        INTERN_ERROR_LOCATION(__FILE__, __LINE__);
        fprintf(stderr, "Tried accessing list with size of %d at index %d\n", list->size, index);
        exit(1);
    }

    return list->elements + (index * list->elementSize);
}