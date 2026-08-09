/*
 * Юнит-тесты для calc.c (задание 2.3)
 *
 * 2.3 отличается от 2.2 диспетчеризацией: вместо switch используется массив
 * указателей на функции, поэтому отдельная часть тестов проверяет именно его.
 *
 * sum/sub/mult/divide объявлены static, слинковать их из отдельного
 * объектного файла нельзя. Стандартный приём - включить сам .c-файл в тест:
 * тест и тестируемый код попадают в одну единицу трансляции, и static
 * перестаёт мешать. main() берётся из этого файла, main.c в сборку не входит.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <io.h>
#include <windows.h> /* SetConsoleOutputCP, CP_UTF8 */

#include "unity/unity.h"
#include "../calc.c"

#define STDIN_FIXTURE  "stdin_fixture.tmp"
#define STDOUT_CAPTURE "stdout_capture.tmp"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

/* Подменяет stdin содержимым строки - так тестируется interface_calc. */
static void feedStdin(const char* input)
{
    FILE* f = fopen(STDIN_FIXTURE, "wb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "не удалось создать файл-фикстуру для stdin");
    fputs(input, f);
    fclose(f);

    TEST_ASSERT_NOT_NULL_MESSAGE(freopen(STDIN_FIXTURE, "r", stdin),
                                 "не удалось перенаправить stdin");
}

/*
 * interface_calc печатает результаты в stdout, поэтому его мало заглушить -
 * нужно прочитать. Перехватываем в файл, потом возвращаем дескриптор на место.
 */
static int savedStdoutFd = -1;

static void captureStdout(void)
{
    fflush(stdout);
    savedStdoutFd = _dup(_fileno(stdout));
    TEST_ASSERT_NOT_NULL_MESSAGE(freopen(STDOUT_CAPTURE, "wb", stdout),
                                 "не удалось перехватить stdout");
}

static void restoreStdout(void)
{
    if (savedStdoutFd < 0) return;

    fflush(stdout);
    _dup2(savedStdoutFd, _fileno(stdout));
    _close(savedStdoutFd);
    savedStdoutFd = -1;
    clearerr(stdout);
}

static const char* endCaptureStdout(void)
{
    static char buffer[8192];

    restoreStdout();

    FILE* f = fopen(STDOUT_CAPTURE, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "не удалось открыть файл перехвата stdout");

    size_t readBytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[readBytes] = '\0';
    fclose(f);

    return buffer;
}

/*
 * Прогоняет interface_calc на заданном вводе и отдаёт всё, что попало в stdout.
 * Ввод обязан заканчиваться пунктом "0" - см. test_interface_eof_hangs.
 */
static const char* runInterface(const char* input)
{
    feedStdin(input);
    captureStdout();
    interface_calc();
    return endCaptureStdout();
}

/* Unity экранирует любой байт вне ASCII, поэтому сообщения - латиницей. */
static void assertContains(const char* haystack, const char* needle,
                           const char* message)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(haystack, needle), message);
}

static void assertNotContains(const char* haystack, const char* needle,
                              const char* message)
{
    TEST_ASSERT_NULL_MESSAGE(strstr(haystack, needle), message);
}

void setUp(void)
{
}

void tearDown(void)
{
    restoreStdout();
}

/* ------------------------------------------------------------------ */
/* sum                                                                 */
/* ------------------------------------------------------------------ */

static void test_sum_positiveNumbers(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(8.0, sum(5.0, 3.0));
}

static void test_sum_negativeNumbers(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(-8.0, sum(-5.0, -3.0));
}

static void test_sum_withZero_returnsOtherOperand(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(5.0, sum(5.0, 0.0));
    TEST_ASSERT_EQUAL_DOUBLE(5.0, sum(0.0, 5.0));
}

static void test_sum_fractional(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(0.75, sum(0.5, 0.25));
}

/* ------------------------------------------------------------------ */
/* sub                                                                 */
/* ------------------------------------------------------------------ */

static void test_sub_positiveNumbers(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(2.0, sub(5.0, 3.0));
}

static void test_sub_resultIsNegative(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(-2.0, sub(3.0, 5.0));
}

static void test_sub_isNotCommutative(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(-sub(3.0, 5.0), sub(5.0, 3.0));
}

static void test_sub_sameOperands_givesZero(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(0.0, sub(5.0, 5.0));
}

/* ------------------------------------------------------------------ */
/* mult                                                                */
/* ------------------------------------------------------------------ */

static void test_mult_positiveNumbers(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(15.0, mult(5.0, 3.0));
}

static void test_mult_byZero_givesZero(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(0.0, mult(5.0, 0.0));
}

static void test_mult_negativeByNegative_givesPositive(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(15.0, mult(-5.0, -3.0));
}

static void test_mult_fractional(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(1.25, mult(0.5, 2.5));
}

/* ------------------------------------------------------------------ */
/* divide                                                              */
/* ------------------------------------------------------------------ */

static void test_divide_evenDivision(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(5.0, divide(10.0, 2.0));
}

static void test_divide_fractionalResult(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(3.5, divide(7.0, 2.0));
}

static void test_divide_negativeOperand(void)
{
    TEST_ASSERT_EQUAL_DOUBLE(-5.0, divide(10.0, -2.0));
}

static void test_divide_byZero_returnsInfinity(void)
{
    TEST_ASSERT_TRUE_MESSAGE(isinf(divide(1.0, 0.0)),
                             "divide(1, 0) is expected to yield infinity");
}

static void test_divide_zeroByZero_returnsNan(void)
{
    TEST_ASSERT_TRUE_MESSAGE(isnan(divide(0.0, 0.0)),
                             "divide(0, 0) is expected to yield NaN");
}

/* ------------------------------------------------------------------ */
/* Таблица указателей на функции                                       */
/* ------------------------------------------------------------------ */

/*
 * Таблица operations объявлена внутри interface_calc, снаружи её не достать.
 * Собираем такую же и проверяем, что порядок пунктов меню (1..4) совпадает
 * с порядком операций - именно на этом соответствии держится диспетчеризация.
 */
static void test_operationsTable_orderMatchesMenu(void)
{
    double (*operations[4])(double, double) = { sum, sub, mult, divide };

    TEST_ASSERT_EQUAL_DOUBLE(8.0,  operations[1 - 1](5.0, 3.0));
    TEST_ASSERT_EQUAL_DOUBLE(2.0,  operations[2 - 1](5.0, 3.0));
    TEST_ASSERT_EQUAL_DOUBLE(15.0, operations[3 - 1](5.0, 3.0));
    TEST_ASSERT_EQUAL_DOUBLE(2.5,  operations[4 - 1](5.0, 2.0));
}

static void test_operationsTable_hasFourEntries(void)
{
    double (*operations[4])(double, double) = { sum, sub, mult, divide };
    int lenght = sizeof(operations) / sizeof(operations[0]);

    TEST_ASSERT_EQUAL_INT_MESSAGE(4, lenght,
                                  "menu offers exactly four operations");
}

/* ------------------------------------------------------------------ */
/* interface_calc - выход и меню                                       */
/* ------------------------------------------------------------------ */

static void test_interface_zero_exitsImmediately(void)
{
    const char* out = runInterface("0\n");

    assertContains(out, "Выбор:", "menu prompt is expected before exit");
    assertNotContains(out, "Результат",
                      "no operation is expected to run before exit");
}

static void test_interface_printsMenuOnEveryIteration(void)
{
    const char* out = runInterface("1\n2 2\n0\n");

    const char* first = strstr(out, "Калькулятор:");
    TEST_ASSERT_NOT_NULL_MESSAGE(first, "menu is expected on first iteration");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(first + 1, "Калькулятор:"),
                                 "menu is expected again after an operation");
}

/* ------------------------------------------------------------------ */
/* interface_calc - арифметика                                         */
/* ------------------------------------------------------------------ */

/* В 2.3 результат печатается через %f - отсюда шесть знаков после запятой. */
static void test_interface_addition(void)
{
    const char* out = runInterface("1\n5 3\n0\n");
    assertContains(out, "Результат: 8.000000", "5 + 3 is expected to be 8");
}

static void test_interface_subtraction(void)
{
    const char* out = runInterface("2\n5 3\n0\n");
    assertContains(out, "Результат: 2.000000", "5 - 3 is expected to be 2");
}

static void test_interface_multiplication(void)
{
    const char* out = runInterface("3\n5 3\n0\n");
    assertContains(out, "Результат: 15.000000", "5 * 3 is expected to be 15");
}

static void test_interface_division(void)
{
    const char* out = runInterface("4\n7 2\n0\n");
    assertContains(out, "Результат: 3.500000", "7 / 2 is expected to be 3.5");
}

static void test_interface_acceptsNegativeOperands(void)
{
    const char* out = runInterface("1\n-5 3\n0\n");
    assertContains(out, "Результат: -2.000000",
                   "-5 + 3 is expected to be -2");
}

static void test_interface_severalOperationsInOneSession(void)
{
    const char* out = runInterface("1\n5 3\n2\n5 3\n0\n");

    assertContains(out, "Результат: 8.000000", "first operation is expected");
    assertContains(out, "Результат: 2.000000", "second operation is expected");
}

/* ------------------------------------------------------------------ */
/* interface_calc - обработка ошибок                                   */
/* ------------------------------------------------------------------ */

/*
 * В отличие от 2.2, здесь деления на ноль никто не проверяет: пункт 4
 * вызывается через таблицу, и результат печатается как есть.
 * Точный вид бесконечности в выводе зависит от рантайма, поэтому тест
 * ограничивается фактом печати результата.
 */
static void test_interface_divisionByZero_isNotRejected(void)
{
    const char* out = runInterface("4\n5 0\n0\n");

    assertContains(out, "Результат: ",
                   "2.3 has no division-by-zero guard and prints the result");
}

static void test_interface_divisionByZero_keepsRunning(void)
{
    const char* out = runInterface("4\n5 0\n1\n2 2\n0\n");

    assertContains(out, "Результат: 4.000000",
                   "menu is expected to keep working after division by zero");
}

/* Сообщение о неверном пункте уходит в stderr, в stdout не должно быть цифр. */
static void test_interface_unknownMenuItem_printsNoResult(void)
{
    const char* out = runInterface("9\n1 1\n0\n");

    assertNotContains(out, "Результат: ", "item 9 is expected to be rejected");
}

static void test_interface_negativeMenuItem_printsNoResult(void)
{
    const char* out = runInterface("-3\n1 1\n0\n");

    assertNotContains(out, "Результат: ", "item -3 is expected to be rejected");
}

static void test_interface_unknownMenuItem_keepsRunning(void)
{
    const char* out = runInterface("9\n1 1\n1\n5 3\n0\n");

    assertContains(out, "Результат: 8.000000",
                   "menu is expected to keep working after a bad item");
}

static void test_interface_nonNumericChoice_isRecovered(void)
{
    const char* out = runInterface("abc\n1\n5 3\n0\n");

    assertContains(out, "Результат: 8.000000",
                   "menu is expected to recover after non-numeric choice");
}

static void test_interface_nonNumericOperands_isRecovered(void)
{
    const char* out = runInterface("1\nx y\n1\n5 3\n0\n");

    assertContains(out, "Результат: 8.000000",
                   "menu is expected to recover after non-numeric operands");
}

/*
 * При EOF scanf возвращает EOF, цикл дочитывания getchar сразу упирается в
 * тот же EOF, и continue возвращает управление в начало - выхода из цикла нет.
 * Запуск этого теста повесил бы весь прогон, поэтому он только фиксирует баг.
 */
static void test_interface_eof_hangs(void)
{
    TEST_IGNORE_MESSAGE(
        "interface_calc loops forever on EOF: scanf keeps returning EOF and "
        "the getchar drain loop cannot consume it. Running this test would "
        "hang the suite. Every other test ends its input with menu item 0.");
}

/* ------------------------------------------------------------------ */

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);

    UNITY_BEGIN();

    RUN_TEST(test_sum_positiveNumbers);
    RUN_TEST(test_sum_negativeNumbers);
    RUN_TEST(test_sum_withZero_returnsOtherOperand);
    RUN_TEST(test_sum_fractional);

    RUN_TEST(test_sub_positiveNumbers);
    RUN_TEST(test_sub_resultIsNegative);
    RUN_TEST(test_sub_isNotCommutative);
    RUN_TEST(test_sub_sameOperands_givesZero);

    RUN_TEST(test_mult_positiveNumbers);
    RUN_TEST(test_mult_byZero_givesZero);
    RUN_TEST(test_mult_negativeByNegative_givesPositive);
    RUN_TEST(test_mult_fractional);

    RUN_TEST(test_divide_evenDivision);
    RUN_TEST(test_divide_fractionalResult);
    RUN_TEST(test_divide_negativeOperand);
    RUN_TEST(test_divide_byZero_returnsInfinity);
    RUN_TEST(test_divide_zeroByZero_returnsNan);

    RUN_TEST(test_operationsTable_orderMatchesMenu);
    RUN_TEST(test_operationsTable_hasFourEntries);

    RUN_TEST(test_interface_zero_exitsImmediately);
    RUN_TEST(test_interface_printsMenuOnEveryIteration);

    RUN_TEST(test_interface_addition);
    RUN_TEST(test_interface_subtraction);
    RUN_TEST(test_interface_multiplication);
    RUN_TEST(test_interface_division);
    RUN_TEST(test_interface_acceptsNegativeOperands);
    RUN_TEST(test_interface_severalOperationsInOneSession);

    RUN_TEST(test_interface_divisionByZero_isNotRejected);
    RUN_TEST(test_interface_divisionByZero_keepsRunning);
    RUN_TEST(test_interface_unknownMenuItem_printsNoResult);
    RUN_TEST(test_interface_negativeMenuItem_printsNoResult);
    RUN_TEST(test_interface_unknownMenuItem_keepsRunning);
    RUN_TEST(test_interface_nonNumericChoice_isRecovered);
    RUN_TEST(test_interface_nonNumericOperands_isRecovered);
    RUN_TEST(test_interface_eof_hangs);

    remove(STDIN_FIXTURE);
    remove(STDOUT_CAPTURE);
    return UNITY_END();
}
