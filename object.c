#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

static ObjString* allocateString(char* chars, u32 length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString* takeString(char* chars, u32 length) {
    return allocateString(chars, length);
}

ObjString* copyString(const char* chars, u32 length) {
    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}

void printObject(Value value) {
    switch (value.as.obj->type) {
        case OBJ_STRING:
            printf("%s", cstringFrom(value));
            break;
    }
}
