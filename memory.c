#include <stdlib.h>

#include "memory.h"

void* reallocate(void* pointer, usize oldSize, usize newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(SYSERR);
    return result;
}
