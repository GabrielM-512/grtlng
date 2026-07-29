#include "HashMap.h"

#include <stdlib.h>
#include <string.h>

#include "ArrayList.h"
#include "../global.h"

#define LOAD_FACTOR 0.75

#define PACKET_SIZE (sizeof(Key) + map->valueSize)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(size, pointer, newCount) (realloc((pointer), (size) * (newCount)))

typedef struct {
    char *name;
} Key;

u32 hashString(const char* key) {
    u32 hash = 2166136261u;
    const u16 length = strlen(key);
    for (int i = 0; i < length; i++) {
        hash ^= (u8) key[i];
        hash *= 16777619;
    }
    return hash;
}

void *getEntry(void *entries, u16 capacity, u16 valueSize, const char* key) {

    const u32 hash = hashString(key);
    u32 index = hash % capacity;

    while (true) {
        Key* current = entries + (sizeof(key) + valueSize) * index;
        if (current->name == nullptr) {
            return current;
        }

        if (strcmp(key, current->name) == 0) {
            return current;
        }

        index = (index + 1) % capacity;
    }

}

void HashMapInit(HashMap *map, size_t valueSize) {
    map->capacity = 0;
    map->count = 0;
    map->valueSize = valueSize;
    map->values = nullptr;
}

void HashMapFree(HashMap *map) {
    free(map->values);
    HashMapInit(map, 0);
}

void *getEntryPointer(Key* key) {
    return (void*) key + sizeof(Key);
}

void adjustCapacity(HashMap *map, u16 capacity) {

    void *new = malloc(PACKET_SIZE * capacity);

    for (int i = 0; i < capacity; i++) {
        Key *key = new + PACKET_SIZE * i;
        key->name = nullptr;
    }

    for (int i = 0; i < map->capacity; i++) {
        Key *old = map->values + PACKET_SIZE * i;
        if (old->name == nullptr) continue;

        void *dest = getEntry(new, capacity, map->valueSize, old->name);

        memcpy(dest, old, PACKET_SIZE);
    }

    map->capacity = capacity;

    free(map->values);
    map->values = new;
}

/** @brief
 *  Inserts a new value into the hashmap
 *  @returns true if a new value was inserted, false if an old one was overwritten
 */

bool HashMapSet(HashMap *map, char *key, const void *value) {
    if (map->count + 1 > map->capacity * LOAD_FACTOR) {
        const u16 capacity = GROW_CAPACITY(map->capacity);
        adjustCapacity(map, capacity);
    }

    Key *entry = getEntry(map->values, map->capacity, map->valueSize, key);

    const bool isNewKey = entry->name == nullptr;

    if (isNewKey) map->count++;

    void *dest = getEntryPointer(entry);

    entry->name = key;
    memcpy(dest, value, map->valueSize);

    return isNewKey;

}

/**
 * @brief
 * Gets a value from a hashmap
 * @param map The map the value (may) be contained in
 * @param value A pointer to the variable which the value will be written into
 * @param key The string which serves as the identifier for the value
 * @returns true if the value was in the hashmap, false if it wasn't. If returns false, param value is not changed
 *
 */

bool HashMapGet(const HashMap *map, const char *key, void *value) {
    if (map->capacity == 0) return false;

    Key *entry = getEntry(map->values, map->capacity, map->valueSize, key);

    // not in map
    if (entry->name == nullptr) return false;

    void *source = getEntryPointer(entry);
    memcpy(value, source, map->valueSize);
    return true;
}

/**
 * Whether a certain key has a value associated with it
 * @param map the map to search in
 * @param key the key to search for
 * @return true = value exists in map, false = value does not exist in map
 */
bool HashMapHas(HashMap *map, char *key) {
    if (map->capacity == 0) return false;

    Key *entry = getEntry(map->values, map->capacity, map->valueSize, key);

    return entry->name != nullptr;
}


ArrayList *HashMapAll(HashMap *map) {
    ArrayList *contents = ArrayListNew(map->valueSize);
    for (u16 i = 0; i < map->capacity; i++) {
        Key *key = map->values + PACKET_SIZE * i;
        // only copy non-empty values
        if (key->name == nullptr) continue;

        void *source = getEntryPointer(key);
        ArrayListAdd(contents, source);
    }
    return contents;
}