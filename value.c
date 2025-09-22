#include <stdio.h>

#include "memory.h"
#include "value.h"

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:  return a.as.boolean == b.as.boolean;
        case VAL_NIL:   return true;
        case VAL_NUMBER:return a.as.number == b.as.number;
    }
}

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void appendValueArray(ValueArray* array, Value value) {
    if (array->capacity <= array->count) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(
            Value, array->values, oldCapacity, array->capacity
        );
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(value.as.boolean ? "true" : "false");
            break;
        case VAL_NIL: printf("nil"); break;
        case VAL_NUMBER: printf("%g", value.as.number); break;
    }
}
