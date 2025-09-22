#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "run_table.h"

typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_LESS,
    OP_GREATER,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

typedef struct {
    u32 count;
    u32 capacity;
    u8* code;
    RunTable runTable;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void appendChunk(Chunk* chunk, u8 byte, u32 line);
u32 addConstant(Chunk* chunk, Value value);

#endif
