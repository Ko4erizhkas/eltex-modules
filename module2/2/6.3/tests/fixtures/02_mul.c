/* Фикстура для тестов загрузчика: вторая корректная библиотека операции */

#include "plugin.h"

OP_EXPORT const char op_name[] = "TEST mul";

OP_EXPORT double op_apply(double n1, double n2)
{
    return n1 * n2;
}
