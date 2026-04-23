#pragma once

#include <stddef.h>

#include "../global.h"


/*
 * A tool for decreasing heap fragmentation while also decreasing memory efficiency.
 * Each Arena stores its own size and a pointer to another Arena, forming a linked list of Arenas.
 * When one Arena runs out of space, it will prompt the next (its "child") to allocate the request instead.
 * Child Arenas are allocated automatically. All children are freed when their parent is freed using ArenaFree().
 *
 * The amount of data (in bytes) an Arena is created with may be specified using #define ARENA_ALLOC_SIZE <size>
 *
 * Functions:
 *
 * ArenaAllocator *ArenaNew(); Creates a new Arena
 * void ArenaFree(ArenaAllocator *arena); Frees an Arena and all its children

 * void *ArenaAlloc(ArenaAllocator *arena, size_t size); Allocates memory on the given Arena or one of its children
 */
typedef struct ArenaAllocator {
    u16 capacity, size;
    struct ArenaAllocator *next;
    u8 data[];
} ArenaAllocator;

ArenaAllocator *ArenaNew();
void ArenaFree(ArenaAllocator *arena);

void *ArenaAlloc(ArenaAllocator *arena, size_t size);
