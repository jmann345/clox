#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "hash_table.h"
#include "value.h"

#define HASHTABLE_MAX_LOAD 0.75

void initHashTable(HashTable *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeHashTable(HashTable *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initHashTable(table);
}

static Entry* findEntry(Entry* entries, u32 capacity, ObjString* key) {
    Entry* tombstone = NULL;
    for (u32 index = key->hash % capacity;;index = (index + 1) % capacity) {
        Entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (entry->value.type == VAL_NIL) {
                return tombstone != NULL ? tombstone : entry;
            } else if (tombstone == NULL) {
                tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }
    }
}

static void adjustCapacity(HashTable* table, u32 capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);
    for (u32 i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (u32 i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

GetResult hashTableGet(const HashTable* table, ObjString* key) {
    GetResult result = { .value = NIL_VAL, .found = false };

    if (table->count == 0) return result;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return result;

    result.value = entry->value;
    result.found = true;

    return result;
}

bool hashTableSet(HashTable* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * HASHTABLE_MAX_LOAD) {
        u32 capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool hashTableDelete(HashTable* table, ObjString* key) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

// Merge "from" into "to"
void mergeHashTables(HashTable* from, HashTable* to) {
    for (u32 i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            hashTableSet(to, entry->key, entry->value);
        }
    }
}

ObjString* hashTableFindString(HashTable* table, const char* chars,
                               u32 length, u32 hash) {
    if (table->count == 0) return NULL;

    for (u32 index = hash % table->capacity;;
        index = (index + 1) % table->capacity) {
        Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            if (entry->value.type == VAL_NIL) return NULL;
        } else if (entry->key->length == length &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            // Found it!
            return entry->key;
        }
    }
}
