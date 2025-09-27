#ifndef clox_hash_table_h
#define clox_hash_table_h

#include "common.h"
#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    Value value;
    bool found;
} GetResult;

typedef struct {
    u32 count;
    u32 capacity;
    Entry* entries;
} HashTable;

void initHashTable(HashTable* table);
void freeHashTable(HashTable* table);
GetResult hashTableGet(const HashTable* table, ObjString* key);
bool hashTableSet(HashTable* table, ObjString* key, Value value);
bool hashTableDelete(HashTable* table, ObjString* key);
void mergeHashTables(HashTable* from, HashTable* to);
ObjString* hashTableFindString(HashTable* table, const char* chars,
                               u32 length, u32 hash);

#endif
