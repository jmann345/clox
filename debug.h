#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
u32 disassembleInstruction(Chunk* chunk, u32 offset);

#endif
