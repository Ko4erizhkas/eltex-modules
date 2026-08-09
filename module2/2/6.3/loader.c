#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "loader.h"

#ifdef _WIN32
#include <windows.h>
#define LIB_EXT ".dll"
#else
#include <dlfcn.h>
#define LIB_EXT ".so"
#endif


static LibHandle lib_open(const char* path);
static void* lib_symbol(LibHandle handle, const char* name);
static void lib_close(LibHandle handle);
static void lib_error(const char* path);

static int has_lib_extension(const char* name);
static int ops_load_file(OperationTable* table, const char* dir, const char* file);
static int op_cmp(const void* a, const void* b);


static LibHandle lib_open(const char* path)
{
#ifdef _WIN32
    return (LibHandle)LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW);
#endif
}

static void* lib_symbol(LibHandle handle, const char* name)
{
#ifdef _WIN32
    return (void*)GetProcAddress((HMODULE)handle, name);
#else
    return dlsym(handle, name);
#endif
}

static void lib_close(LibHandle handle)
{
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

static void lib_error(const char* path)
{
#ifdef _WIN32
    fprintf(stderr, "Не удалось загрузить %s (код ошибки %lu)\n",
            path, (unsigned long)GetLastError());
#else
    fprintf(stderr, "Не удалось загрузить %s: %s\n", path, dlerror());
#endif
}

static int has_lib_extension(const char* name)
{
    size_t nameLen = strlen(name);
    size_t extLen = strlen(LIB_EXT);

    if (nameLen <= extLen) return 0;

    return strcmp(name + nameLen - extLen, LIB_EXT) == 0;
}

static int ops_load_file(OperationTable* table, const char* dir, const char* file)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%s", dir, file);

    LibHandle handle = lib_open(path);
    if (handle == NULL)
    {
        lib_error(path);
        return 0;
    }

    /* op_name - экспортированная строка, op_apply - сама операция */
    const char* name = (const char*)lib_symbol(handle, OP_NAME_SYMBOL);
    OpApplyFn apply = (OpApplyFn)lib_symbol(handle, OP_APPLY_SYMBOL);

    if (name == NULL || apply == NULL)
    {
        fprintf(stderr, "В библиотеке %s нет символов %s и %s - пропущена\n",
                path, OP_NAME_SYMBOL, OP_APPLY_SYMBOL);
        lib_close(handle);
        return 0;
    }

    Operation* op = &table->items[table->count];

    snprintf(op->file, sizeof(op->file), "%s", file);
    snprintf(op->name, sizeof(op->name), "%s", name);
    op->apply = apply;
    op->handle = handle;

    table->count++;
    return 1;
}

static int op_cmp(const void* a, const void* b)
{
    const Operation* opA = (const Operation*)a;
    const Operation* opB = (const Operation*)b;

    return strcmp(opA->file, opB->file);
}

int ops_load_dir(OperationTable* table, const char* path)
{
    if (table == NULL || path == NULL) return 0;

    table->count = 0;

    DIR* dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr, "Не удалось открыть каталог с библиотеками: %s\n", path);
        return 0;
    }

    const struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (table->count >= MAX_OPERATIONS)
        {
            fprintf(stderr, "Достигнут предел в %d операций, остальные пропущены\n",
                    MAX_OPERATIONS);
            break;
        }

        if (!has_lib_extension(entry->d_name)) continue;

        ops_load_file(table, path, entry->d_name);
    }

    closedir(dir);

    /* readdir не гарантирует порядок обхода - сортируем, чтобы нумерация меню не плавала */
    qsort(table->items, (size_t)table->count, sizeof(Operation), op_cmp);

    return table->count;
}

void ops_unload(OperationTable* table)
{
    if (table == NULL) return;

    for (int i = 0; i < table->count; ++i)
    {
        lib_close(table->items[i].handle);

        table->items[i].handle = NULL;
        table->items[i].apply = NULL;
    }

    table->count = 0;
}
