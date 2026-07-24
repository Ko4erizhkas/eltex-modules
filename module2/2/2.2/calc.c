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

        switch(choice)
        {
            case 1: 
                printf("Результат сложения: %g\n", sum(a, b));
                break;
            case 2: 
                printf("Результат вычитания: %g\n", sub(a, b));
                break;
            case 3:
                printf("Результат умножения: %g\n", mult(a, b)); 
                break;
            case 4:
                if (b == 0)
                {
                    fprintf(stderr, "Деление на ноль\n");
                }
                else
                { 
                    printf("Результат деления: %g\n", divide(a, b));
                }
                break;
            default: printf("Неверный пункт\n");
        }
    }
}