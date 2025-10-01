#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "hash_table.h"
#include "value.h"
#include "vm.h"

extern VM vm;

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(usize size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;

    return object;
}

static ObjString* allocateString(char* chars, u32 length, u32 hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    hashTableSet(&vm.strings, string, NIL_VAL);
    return string;
}

static u32 hashString(const char* key, u32 length) {
    u32 hash = 2166136261u;
    for (u32 i = 0; i < length; i++) {
        hash ^= (u8)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* takeString(char* chars, u32 length) {
    u32 hash = hashString(chars, length);
    ObjString* interned = hashTableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, u32 length) {
    u32 hash = hashString(chars, length);
    ObjString* interned = hashTableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

void printObject(Value value) {
    switch (value.as.obj->type) {
        case OBJ_STRING:
            printf("%s", cstringFrom(value));
            break;
    }
}
