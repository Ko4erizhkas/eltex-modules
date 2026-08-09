#ifndef LOADER_H
#define LOADER_H

#include "plugin.h"

#define MAX_OPERATIONS 16
#define MAX_NAME 64
#define MAX_PATH_LEN 512

/* И HMODULE в Windows, и результат dlopen в Linux - указатели,
   поэтому наружу хватает void* */
typedef void* LibHandle;

typedef struct Operation
{
    char file[MAX_NAME];
    char name[MAX_NAME];
    OpApplyFn apply;
    LibHandle handle;

} Operation;

typedef struct OperationTable
{
    Operation items[MAX_OPERATIONS];
    int count;

} OperationTable;

int ops_load_dir(OperationTable* table, const char* path);
void ops_unload(OperationTable* table);

#endif
