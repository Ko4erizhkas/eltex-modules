#include <stdio.h>

#include "calc.h"

void interface_calc(const OperationTable* table)
{
    if (table == NULL || table->count == 0) return;

    for (;;)
    {
        printf("\nКалькулятор: \n");
        for (int i = 0; i < table->count; ++i)
        {
            printf("%d) %s\n", i + 1, table->items[i].name);
        }
        printf("0) Выход\n\n");

        printf("Выбор: ");
        int choice;
        if (scanf("%d", &choice) != 1)
        {
            int c;
            fprintf(stderr, "Введите число\n");
            while ((c = getchar()) != '\n' && c != EOF) { }
            continue;
        }
        if (choice == 0) return;

        if (choice < 1 || choice > table->count)
        {
            fprintf(stderr, "Нет такого пункта\n");
            continue;
        }

        double a, b;
        printf("Введите два числа: ");
        if (scanf("%lf %lf", &a, &b) != 2)
        {
            int c;
            fprintf(stderr, "Введите два числа\n");
            while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }

        /* вызов через указатель, полученный из библиотеки при запуске */
        printf("Результат: %f\n", table->items[choice - 1].apply(a, b));
    }
}
