#pragma once

#include <stddef.h>

#include "../global.h"


/*
 * A tool for decreasing heap fragmentation while also decreasing memory efficiency.
 * Each Arena stores its own size and a pointer to another Arena, forming a linked list of Arenas.
 * When one Arena runs out of space, it will prompt the next (its "child") to allocate the request instead.
 * Child Arenas are allocated automatically. All children are freed when their parent is freed using ArenaAllocFree().
 *
 * The amount of data (in bytes) an Arena is created with may be specified using #define ARENA_ALLOC_SIZE <size>
 *
 * Functions:
 *
 * ArenaAlloc *ArenaAllocNew(); Creates a new Arena
 * void ArenaAllocFree(ArenaAlloc *arena); Frees an Arena and all its children

 * void *ArenaAllocAlloc(ArenaAlloc *arena, size_t size); Allocates memory on the given Arena or one of its children
 */
typedef struct ArenaAlloc {
    u16 capacity, size;
    struct ArenaAlloc* next;
    u8 data[];
} ArenaAlloc;

ArenaAlloc *ArenaAllocNew();
void ArenaAllocFree(ArenaAlloc *arena);

void *ArenaAllocAlloc(ArenaAlloc *arena, size_t size);
