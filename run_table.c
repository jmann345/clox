#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "memory.h"

typedef struct { u32 line, len; } Run;
typedef struct {
    Run* runs; 
    u32 capacity; 
    u32 count;
} RunTable;

void initRunTable(RunTable* runTable) {
    runTable->runs = NULL;
    runTable->capacity = 0;
    runTable->count = 0;
}

void appendRunTable(RunTable* runTable, u32 line) {
    if (runTable->count > 0 && 
        runTable->runs[runTable->count-1].line == line) {
        runTable->runs[runTable->count-1].len++;
        return;
    }

    // If we haven't returned yet, resize if needed add a new run entry
    if (runTable->capacity <= runTable->count) {
        u32 oldCapacity = runTable->capacity;
        runTable->capacity = GROW_CAPACITY(oldCapacity);
        runTable->runs = GROW_ARRAY(
            Run, runTable->runs, oldCapacity, runTable->capacity
        );
    }

    Run run = {.line = line, .len = 1}; 
    runTable->runs[runTable->count++] = run;
}
void freeRunTable(RunTable* runTable) {
    FREE_ARRAY(Run, runTable->runs, runTable->capacity);
    initRunTable(runTable);
}

void printRunTable(const RunTable* runTable) {
    printf("%7s%11s\n", "capacity", "count");
    printf("%7u%11u\n", runTable->capacity, runTable->count);

    printf("%7s\n", "Entries:");
    printf("%7s%11s\n", "line", "length");
    for (u32 i = 0; i < runTable->count; i++) {
        printf("%7u%11u\n", runTable->runs[i].line, runTable->runs[i].len);
    }
}

u32 getLine(const RunTable* runTable, u32 instrIndex) {
    u32 acc = 0;
    for (u32 i = 0; i < runTable->count; i++) {
        acc += runTable->runs[i].len;

        if (acc > instrIndex) {
            return runTable->runs[i].line;
        }
    }
    
    exit(SYSERR);
}
