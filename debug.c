#include <stdio.h>

#include "debug.h"
#include "chunk.h"
#include "value.h"

static u32 simpleInstruction(const char* name, u32 offset) {
    printf("%s\n", name);
    return offset + 1;
}

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (u32 offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static u32 constantInstruction(const char* name, Chunk* chunk, u32 offset) {
    u8 constant = chunk->code[offset+1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

u32 disassembleInstruction(Chunk *chunk, u32 offset) {
    printf("%04u ", offset);
    if (offset > 0 && 
        getLine(&chunk->runTable, offset) == getLine(&chunk->runTable, offset-1)) {
        printf("   | ");
    } else {
        printf("%4u ", getLine(&chunk->runTable, offset));
    }

    u8 instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
