#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "calc.h"

#define DEFAULT_PLUGIN_DIR "plugins"

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    /* каталог с библиотеками можно передать первым аргументом */
    const char* dir = (argc > 1) ? argv[1] : DEFAULT_PLUGIN_DIR;

    OperationTable table;
    if (ops_load_dir(&table, dir) == 0)
    {
        fprintf(stderr, "В каталоге %s не найдено ни одной операции\n", dir);
        return 1;
    }

    printf("Каталог: %s\n", dir);
    printf("Загружено операций: %d\n", table.count);
    for (int i = 0; i < table.count; ++i)
    {
        printf("  %s -> %s\n", table.items[i].file, table.items[i].name);
    }

    interface_calc(&table);
    ops_unload(&table);

    return 0;
}
