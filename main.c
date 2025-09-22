#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
    char line[1024];
    for ever {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    usize fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    usize bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    
cleanup:
    free(source);

    switch(result) {
        case INTERPRET_OK: break; // do nothing
        case INTERPRET_COMPILE_ERROR: exit(65);
        case INTERPRET_RUNTIME_ERROR: exit(70);
    }
}

int main(int argc, const char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

cleanup:
    freeVM();
    return OK;
}

// Chunk chunk;
// initChunk(&chunk);
// //             [-]
// //              |
// //             [/]
// //         ----| |----
// //         |         |
// //        [+]      [5.6]
// //    ----| |----
// //    |         |
// //  [1.2]     [3.4]
// u32 constIndex = addConstant(&chunk, 1.2);
// appendChunk(&chunk, OP_CONSTANT, 123);
// // [OP_CONSTANT][c->constants[const_index]] <- Bytecode format
//
// appendChunk(&chunk, constIndex, 123);
// constIndex = addConstant(&chunk, 3.4);
// appendChunk(&chunk, OP_CONSTANT, 123);
// appendChunk(&chunk, constIndex, 123);
//
// appendChunk(&chunk, OP_ADD, 123);
//
// constIndex = addConstant(&chunk, 5.6);
// appendChunk(&chunk, OP_CONSTANT, 123);
// appendChunk(&chunk, constIndex, 123);
// // Notice: output of OP_ADD implicitly flows into operand of OP_DIVIDE!
// appendChunk(&chunk, OP_DIVIDE, 123);
// appendChunk(&chunk, OP_NEGATE, 123);
//
// appendChunk(&chunk, OP_RETURN, 123);
//
// disassembleChunk(&chunk, "test chunk");
// interpret(&chunk);
// freeChunk(&chunk);
//
