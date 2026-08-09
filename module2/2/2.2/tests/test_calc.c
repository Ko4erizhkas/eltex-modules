/*
 * Юнит-тесты для calc.c (задание 2.2)
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

/*
 * Сама divide деление на ноль не проверяет - от падения защищает только
 * условие в interface_calc. На IEEE-754 это даёт бесконечность, а не UB.
 */
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

static void test_interface_addition(void)
{
    const char* out = runInterface("1\n5 3\n0\n");
    assertContains(out, "Результат сложения: 8", "5 + 3 is expected to be 8");
}

static void test_interface_subtraction(void)
{
    const char* out = runInterface("2\n5 3\n0\n");
    assertContains(out, "Результат вычитания: 2", "5 - 3 is expected to be 2");
}

static void test_interface_multiplication(void)
{
    const char* out = runInterface("3\n5 3\n0\n");
    assertContains(out, "Результат умножения: 15", "5 * 3 is expected to be 15");
}

static void test_interface_division(void)
{
    const char* out = runInterface("4\n7 2\n0\n");
    assertContains(out, "Результат деления: 3.5", "7 / 2 is expected to be 3.5");
}

static void test_interface_acceptsFractionalOperands(void)
{
    const char* out = runInterface("1\n0.5 0.25\n0\n");
    assertContains(out, "Результат сложения: 0.75",
                   "0.5 + 0.25 is expected to be 0.75");
}

static void test_interface_acceptsNegativeOperands(void)
{
    const char* out = runInterface("1\n-5 3\n0\n");
    assertContains(out, "Результат сложения: -2",
                   "-5 + 3 is expected to be -2");
}

static void test_interface_severalOperationsInOneSession(void)
{
    const char* out = runInterface("1\n5 3\n3\n2 4\n0\n");

    assertContains(out, "Результат сложения: 8", "first operation is expected");
    assertContains(out, "Результат умножения: 8", "second operation is expected");
}

/* ------------------------------------------------------------------ */
/* interface_calc - обработка ошибок                                   */
/* ------------------------------------------------------------------ */

/* Единственная защита от деления на ноль живёт здесь, а не в divide. */
static void test_interface_divisionByZero_printsNoResult(void)
{
    const char* out = runInterface("4\n5 0\n0\n");

    assertNotContains(out, "Результат деления",
                      "division by zero is expected to be rejected");
}

static void test_interface_divisionByZero_keepsRunning(void)
{
    const char* out = runInterface("4\n5 0\n1\n2 2\n0\n");

    assertContains(out, "Результат сложения: 4",
                   "menu is expected to keep working after division by zero");
}

static void test_interface_unknownMenuItem_isReported(void)
{
    const char* out = runInterface("9\n1 1\n0\n");
    assertContains(out, "Неверный пункт", "item 9 is expected to be rejected");
}

static void test_interface_negativeMenuItem_isReported(void)
{
    const char* out = runInterface("-3\n1 1\n0\n");
    assertContains(out, "Неверный пункт", "item -3 is expected to be rejected");
}

static void test_interface_nonNumericChoice_isRecovered(void)
{
    const char* out = runInterface("abc\n1\n5 3\n0\n");

    assertContains(out, "Результат сложения: 8",
                   "menu is expected to recover after non-numeric choice");
}

static void test_interface_nonNumericOperands_isRecovered(void)
{
    const char* out = runInterface("1\nx y\n1\n5 3\n0\n");

    assertContains(out, "Результат сложения: 8",
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

    RUN_TEST(test_interface_zero_exitsImmediately);
    RUN_TEST(test_interface_printsMenuOnEveryIteration);

    RUN_TEST(test_interface_addition);
    RUN_TEST(test_interface_subtraction);
    RUN_TEST(test_interface_multiplication);
    RUN_TEST(test_interface_division);
    RUN_TEST(test_interface_acceptsFractionalOperands);
    RUN_TEST(test_interface_acceptsNegativeOperands);
    RUN_TEST(test_interface_severalOperationsInOneSession);

    RUN_TEST(test_interface_divisionByZero_printsNoResult);
    RUN_TEST(test_interface_divisionByZero_keepsRunning);
    RUN_TEST(test_interface_unknownMenuItem_isReported);
    RUN_TEST(test_interface_negativeMenuItem_isReported);
    RUN_TEST(test_interface_nonNumericChoice_isRecovered);
    RUN_TEST(test_interface_nonNumericOperands_isRecovered);
    RUN_TEST(test_interface_eof_hangs);

    remove(STDIN_FIXTURE);
    remove(STDOUT_CAPTURE);
    return UNITY_END();
}
