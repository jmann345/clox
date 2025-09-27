#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"
#include "hash_table.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    u8* ip;  // instruction pointer
    Value stack[STACK_MAX];
    Value* stackTop;
    HashTable strings;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM(void);
void freeVM(void);

InterpretResult interpret(const char* source);

void push(Value value);
Value pop(void);
Value peek(i32 distance);
void replaceTop(Value value);
Value top(void);
Value* top_mut(void);

#endif
