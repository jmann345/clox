#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"
#include "run_table.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    initRunTable(&chunk->runTable);
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(u8, chunk->code, chunk->capacity);
    freeRunTable(&chunk->runTable);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void appendChunk(Chunk* chunk, u8 byte, u32 line) {
    if (chunk->capacity <= chunk->count+1) {
        u32 oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(
            u8, chunk->code, oldCapacity, chunk->capacity
        );
    }

    chunk->code[chunk->count++] = byte;
    appendRunTable(&chunk->runTable, line);
}

u32 addConstant(Chunk *chunk, Value value) {
    appendValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}
