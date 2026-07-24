#include "calc.h"
#include <stdio.h>
static double sum (double n1, double n2) { return n1 + n2; }
static double sub (double n1, double n2) { return n1 - n2; }
static double mult (double n1, double n2) {return n1 * n2; }
static double divide (double n1, double n2)
{
    return n1 / n2;
}

void interface_calc(void)
{
    for (;;)
    {
        int choice;
        printf("Калькулятор: \n1) + \n2) - \n3) * \n4) / \n0)Выход\n\n");
        printf("Выбор: ");
        if (scanf("%d", &choice) != 1) {
            int c;
            fprintf(stderr, "Введите число\n");
            while ((c = getchar()) != '\n' && c != EOF) { }
            continue;
        }
        if (choice == 0) return;

        double a, b;
        printf("Введите два числа: ");
        if (scanf("%lf %lf", &a, &b) != 2)
        {
            int c;
            fprintf(stderr, "Введите два числа: ");
            while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }

        double (*operations[4])(double, double) = {sum, sub, mult, divide};
        int lenght = sizeof(operations) / sizeof(operations[0]);
        if (choice < 1 || choice > lenght)
        {
            fprintf(stderr, "Нет такого пункта\n");
            continue;
        }
        printf("Результат: %f\n", operations[choice - 1](a,b));
    }
}