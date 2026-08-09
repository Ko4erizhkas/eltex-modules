/*
 * Фикстура для тестов загрузчика: библиотека собирается и открывается,
 * но контракта не соблюдает - ни op_name, ни op_apply в ней нет.
 * Загрузчик обязан пропустить её, а не упасть.
 */

#include "plugin.h"

OP_EXPORT double not_an_operation(double n1, double n2)
{
    return n1 - n2;
}
