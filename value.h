#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        f64 number;
        Obj* obj;
    } as;
} Value;

#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL             ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)     ((Value){VAL_OBJ, {.obj = (Obj*)object}})

typedef struct {
    u32 capacity;
    u32 count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void printValue(Value value);
void initValueArray(ValueArray* array);
void appendValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

#endif
