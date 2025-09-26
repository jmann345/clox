#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "memory.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "value.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    usize instrIndex = vm.ip - vm.chunk->code - 1;
    u32 line = getLine(&vm.chunk->runTable, instrIndex);
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
}

void freeVM() {
    freeObjects();
}

void push(Value value) {
    *vm.stackTop++ = value;
}

Value pop() {
    return *--vm.stackTop;
}

Value peek(i32 distance) {
    return vm.stackTop[-1 - distance];
}

void replaceTop(Value value) {
    vm.stackTop[-1] = value;
}

Value top() {
    return vm.stackTop[-1];
}

Value* top_mut() {
    return &vm.stackTop[-1];
}

static inline void concatenate() {
    ObjString* b = stringFrom(pop());
    ObjString* a = stringFrom(pop());

    u32 length = a->length + b->length;
    char* chars = ALLOCATE(char, length+1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(valueType, op) \
    do { \
        if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = pop().as.number; \
        double a = pop().as.number; \
        push(valueType(a op b)); \
    } while (false)

    for ever {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk, (u32)(vm.ip - vm.chunk->code));
#endif
        u8 instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                printValue(constant);
                printf("\n");
                break;
            }
            case OP_NIL:        push(NIL_VAL);              break;
            case OP_TRUE:       push(BOOL_VAL(true));       break;
            case OP_FALSE:      push(BOOL_VAL(false));      break;
            case OP_EQUAL: {
                Value rhs = pop();
                Value lhs = pop();
                push(BOOL_VAL(valuesEqual(lhs, rhs)));
                break;
            }
            case OP_LESS:       BINARY_OP(BOOL_VAL, <);   break;
            case OP_GREATER:    BINARY_OP(BOOL_VAL, >);   break;
            case OP_ADD: {
                if (isObjType(peek(0), OBJ_STRING) && isObjType(peek(1), OBJ_STRING)) {
                    concatenate();
                } else if (peek(0).type == VAL_NUMBER && peek(1).type == VAL_NUMBER) {
                    f64 b = pop().as.number;
                    f64 a = pop().as.number;
                    push(NUMBER_VAL(a+b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -);   break;
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *);   break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /);   break;
            case OP_NOT: {
                if (top().type != VAL_BOOL) {
                    runtimeError("operand must be a boolean.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value* top = top_mut();
                top->as.boolean = !top->as.boolean;
                break;
            }
            case OP_NEGATE: {
                if (top().type != VAL_NUMBER) {
                    runtimeError("operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value* top = top_mut();
                top->as.number = -top->as.number;
                break;
            }
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    InterpretResult result;
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        result = INTERPRET_COMPILE_ERROR;
        goto cleanup;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    result = run();

cleanup:
    freeChunk(&chunk);
    return result;
}
