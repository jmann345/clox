#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    u32 length;
    char* chars;
};

ObjString* takeString(char* chars, u32 length);
ObjString* copyString(const char* chars, u32 length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return value.type == VAL_OBJ && value.as.obj->type == type;
}

static inline ObjString* stringFrom(Value value) {
    return (ObjString*)value.as.obj;
}
static inline char* cstringFrom(Value value) {
    return ((ObjString*)value.as.obj)->chars;
}

#endif
