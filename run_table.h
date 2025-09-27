#ifndef clox_run_table_h
#define clox_run_table_h

#include "common.h"

typedef struct { u32 line, len; } Run;
typedef struct {
    Run* runs; 
    u32 capacity; 
    u32 count;
} RunTable;

void initRunTable(RunTable* runTable);
void appendRunTable(RunTable* runTable, u32 line);
void freeRunTable(RunTable* runTable);
void printRunTable(RunTable* runTable);

u32 getLine(RunTable* runTable, u32 instrIndex);

#endif
