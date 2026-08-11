/*
 * Юнит-тесты для pqueue.c (задание 4.2)
 *
 * Очередь целиком доступна через публичный API, поэтому pqueue.c линкуется с
 * тестом как обычный объектный файл, включать .c не нужно. main() берётся из
 * этого файла, main.c в сборку тестов не входит. Единственная функция с
 * выводом - pq_print, для неё stdout перехватывается в файл.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h> /* SetConsoleOutputCP, CP_UTF8 */
#define DUP _dup
#define DUP2 _dup2
#define FILENO _fileno
#define CLOSE _close
#else
#include <unistd.h>
#define DUP dup
#define DUP2 dup2
#define FILENO fileno
#define CLOSE close
#endif

#include "unity/unity.h"
#include "../pqueue.h"

#define STDOUT_CAPTURE "stdout_capture.tmp"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

static PQueue* queue = NULL;

/*
 * pq_print печатает в stdout, поэтому его мало заглушить - нужно прочитать.
 * Перехватываем в файл, потом возвращаем дескриптор на место.
 */
static int savedStdoutFd = -1;

static void captureStdout(void)
{
    fflush(stdout);
    savedStdoutFd = DUP(FILENO(stdout));
    TEST_ASSERT_NOT_NULL_MESSAGE(freopen(STDOUT_CAPTURE, "wb", stdout),
                                 "не удалось перехватить stdout");
}

static void restoreStdout(void)
{
    if (savedStdoutFd < 0) return;

    fflush(stdout);
    DUP2(savedStdoutFd, FILENO(stdout));
    CLOSE(savedStdoutFd);
    savedStdoutFd = -1;
    clearerr(stdout);
}

static const char* runPrint(const PQueue* pq)
{
    static char buffer[4096];
    size_t readBytes;
    FILE* f;

    captureStdout();
    pq_print(pq);
    restoreStdout();

    f = fopen(STDOUT_CAPTURE, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "не удалось открыть файл перехвата stdout");

    readBytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[readBytes] = '\0';
    fclose(f);

    return buffer;
}

/* Unity экранирует любой байт вне ASCII, поэтому сообщения - латиницей. */
static void assertContains(const char* haystack, const char* needle,
                           const char* message)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(haystack, needle), message);
}

static void push(const char* text, int priority)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, pq_push(queue, text, priority), text);
}

/* Извлекает сообщение и проверяет, что пришло именно ожидаемое. */
static void assertPop(int found, const Message* msg,
                      const char* expectedText, int expectedPriority)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, found, expectedText);
    TEST_ASSERT_EQUAL_STRING(expectedText, msg->text);
    TEST_ASSERT_EQUAL_INT(expectedPriority, msg->priority);
}

static void assertSize(size_t expected)
{
    TEST_ASSERT_EQUAL_UINT((unsigned)expected, (unsigned)queue->size);
}

void setUp(void)
{
    queue = pq_init();
    TEST_ASSERT_NOT_NULL_MESSAGE(queue, "pq_init returned NULL");
}

void tearDown(void)
{
    restoreStdout();
    pq_free(&queue);
}

/* ------------------------------------------------------------------ */
/* pq_init, pq_free                                                    */
/* ------------------------------------------------------------------ */

static void test_init_queueIsEmpty(void)
{
    TEST_ASSERT_NULL(queue->head);
    assertSize(0);
}

static void test_free_resetsPointer(void)
{
    push("сообщение", 10);

    pq_free(&queue);
    TEST_ASSERT_NULL(queue);
}

/* Повторный вызов и NULL не должны приводить к падению. */
static void test_free_toleratesNullAndSecondCall(void)
{
    pq_free(&queue);
    pq_free(&queue);
    pq_free(NULL);
}

/* ------------------------------------------------------------------ */
/* pq_push                                                             */
/* ------------------------------------------------------------------ */

static void test_push_increasesSize(void)
{
    push("первое", 1);
    assertSize(1);

    push("второе", 2);
    assertSize(2);
}

static void test_push_acceptsWholePriorityRange(void)
{
    push("минимальный", PRIORITY_MIN);
    push("максимальный", PRIORITY_MAX);
    assertSize(2);
}

static void test_push_rejectsPriorityOutOfRange(void)
{
    TEST_ASSERT_EQUAL_INT(0, pq_push(queue, "слишком мало", PRIORITY_MIN - 1));
    TEST_ASSERT_EQUAL_INT(0, pq_push(queue, "слишком много", PRIORITY_MAX + 1));
    assertSize(0);
}

static void test_push_rejectsNullArguments(void)
{
    TEST_ASSERT_EQUAL_INT(0, pq_push(queue, NULL, 10));
    TEST_ASSERT_EQUAL_INT(0, pq_push(NULL, "сообщение", 10));
    assertSize(0);
}

static void test_push_truncatesLongText(void)
{
    char longText[MAX_TEXT * 2];
    Message msg;

    memset(longText, 'a', sizeof(longText) - 1);
    longText[sizeof(longText) - 1] = '\0';

    push(longText, 10);

    TEST_ASSERT_EQUAL_INT(1, pq_pop(queue, &msg));
    TEST_ASSERT_EQUAL_UINT(MAX_TEXT - 1, (unsigned)strlen(msg.text));
}

/* ------------------------------------------------------------------ */
/* Порядок в очереди                                                   */
/* ------------------------------------------------------------------ */

static void test_order_higherPriorityGoesFirst(void)
{
    Message msg;

    push("низкий", 1);
    push("высокий", 200);
    push("средний", 100);

    assertPop(pq_pop(queue, &msg), &msg, "высокий", 200);
    assertPop(pq_pop(queue, &msg), &msg, "средний", 100);
    assertPop(pq_pop(queue, &msg), &msg, "низкий", 1);
    assertSize(0);
}

/* Внутри одного приоритета порядок остаётся живой очередью. */
static void test_order_equalPrioritiesKeepFifo(void)
{
    Message msg;

    push("первое", 50);
    push("второе", 50);
    push("третье", 50);

    assertPop(pq_pop(queue, &msg), &msg, "первое", 50);
    assertPop(pq_pop(queue, &msg), &msg, "второе", 50);
    assertPop(pq_pop(queue, &msg), &msg, "третье", 50);
}

/* ------------------------------------------------------------------ */
/* pq_pop                                                              */
/* ------------------------------------------------------------------ */

static void test_pop_removesElement(void)
{
    Message msg;

    push("сообщение", 10);
    TEST_ASSERT_EQUAL_INT(1, pq_pop(queue, &msg));
    assertSize(0);
    TEST_ASSERT_NULL(queue->head);
}

static void test_pop_emptyQueue_returnsZeroAndKeepsOut(void)
{
    Message msg;
    msg.priority = -5;

    TEST_ASSERT_EQUAL_INT(0, pq_pop(queue, &msg));
    TEST_ASSERT_EQUAL_INT(-5, msg.priority);
    TEST_ASSERT_EQUAL_INT(0, pq_pop(NULL, &msg));
}

/* Сообщение можно просто выбросить, не забирая его содержимое. */
static void test_pop_acceptsNullOut(void)
{
    push("сообщение", 10);

    TEST_ASSERT_EQUAL_INT(1, pq_pop(queue, NULL));
    assertSize(0);
}

/* ------------------------------------------------------------------ */
/* pq_pop_priority                                                     */
/* ------------------------------------------------------------------ */

static void test_popPriority_takesExactMatchFromMiddle(void)
{
    Message msg;

    push("высокий", 200);
    push("нужный", 100);
    push("низкий", 1);

    assertPop(pq_pop_priority(queue, 100, &msg), &msg, "нужный", 100);
    assertSize(2);
    assertPop(pq_pop(queue, &msg), &msg, "высокий", 200);
}

static void test_popPriority_takesFirstOfEqual(void)
{
    Message msg;

    push("первое", 50);
    push("второе", 50);

    assertPop(pq_pop_priority(queue, 50, &msg), &msg, "первое", 50);
}

static void test_popPriority_missing_returnsZeroAndKeepsQueue(void)
{
    Message msg;

    push("сообщение", 10);

    TEST_ASSERT_EQUAL_INT(0, pq_pop_priority(queue, 11, &msg));
    TEST_ASSERT_EQUAL_INT(0, pq_pop_priority(queue, 300, &msg));
    TEST_ASSERT_EQUAL_INT(0, pq_pop_priority(NULL, 10, &msg));
    assertSize(1);
}

/* ------------------------------------------------------------------ */
/* pq_pop_atleast                                                      */
/* ------------------------------------------------------------------ */

static void test_popAtleast_takesFirstNotLowerThanGiven(void)
{
    Message msg;

    push("высокий", 200);
    push("средний", 100);
    push("низкий", 1);

    assertPop(pq_pop_atleast(queue, 150, &msg), &msg, "высокий", 200);
    assertPop(pq_pop_atleast(queue, 50, &msg), &msg, "средний", 100);
    assertSize(1);
}

/* "Не ниже" включает и равный приоритет. */
static void test_popAtleast_thresholdIsInclusive(void)
{
    Message msg;

    push("ровно", 100);

    assertPop(pq_pop_atleast(queue, 100, &msg), &msg, "ровно", 100);
}

static void test_popAtleast_noSuitableMessage_returnsZero(void)
{
    Message msg;

    push("низкий", 10);

    TEST_ASSERT_EQUAL_INT(0, pq_pop_atleast(queue, 11, &msg));
    TEST_ASSERT_EQUAL_INT(0, pq_pop_atleast(NULL, 0, &msg));
    assertSize(1);
}

/* Очередь отсортирована, поэтому порог 0 - это то же самое, что pq_pop. */
static void test_popAtleast_zeroTakesHeadOfQueue(void)
{
    Message msg;

    push("низкий", 1);
    push("высокий", 200);

    assertPop(pq_pop_atleast(queue, PRIORITY_MIN, &msg), &msg, "высокий", 200);
}

/* ------------------------------------------------------------------ */
/* pq_print                                                            */
/* ------------------------------------------------------------------ */

static void test_print_emptyQueue(void)
{
    assertContains(runPrint(queue), "Очередь пуста", "empty queue message is missing");
}

static void test_print_listsMessagesInPriorityOrder(void)
{
    const char* output;
    const char* high;
    const char* low;

    push("низкий", 1);
    push("высокий", 200);

    output = runPrint(queue);
    high = strstr(output, "высокий");
    low = strstr(output, "низкий");

    TEST_ASSERT_NOT_NULL_MESSAGE(high, "high priority message is missing");
    TEST_ASSERT_NOT_NULL_MESSAGE(low, "low priority message is missing");
    TEST_ASSERT_TRUE_MESSAGE(high < low, "messages are printed in wrong order");

    assertContains(output, "200", "priority number is missing");
}

static void test_print_toleratesNull(void)
{
    assertContains(runPrint(NULL), "Очередь пуста", "NULL queue must be reported as empty");
}

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    UNITY_BEGIN();

    RUN_TEST(test_init_queueIsEmpty);
    RUN_TEST(test_free_resetsPointer);
    RUN_TEST(test_free_toleratesNullAndSecondCall);

    RUN_TEST(test_push_increasesSize);
    RUN_TEST(test_push_acceptsWholePriorityRange);
    RUN_TEST(test_push_rejectsPriorityOutOfRange);
    RUN_TEST(test_push_rejectsNullArguments);
    RUN_TEST(test_push_truncatesLongText);

    RUN_TEST(test_order_higherPriorityGoesFirst);
    RUN_TEST(test_order_equalPrioritiesKeepFifo);

    RUN_TEST(test_pop_removesElement);
    RUN_TEST(test_pop_emptyQueue_returnsZeroAndKeepsOut);
    RUN_TEST(test_pop_acceptsNullOut);

    RUN_TEST(test_popPriority_takesExactMatchFromMiddle);
    RUN_TEST(test_popPriority_takesFirstOfEqual);
    RUN_TEST(test_popPriority_missing_returnsZeroAndKeepsQueue);

    RUN_TEST(test_popAtleast_takesFirstNotLowerThanGiven);
    RUN_TEST(test_popAtleast_thresholdIsInclusive);
    RUN_TEST(test_popAtleast_noSuitableMessage_returnsZero);
    RUN_TEST(test_popAtleast_zeroTakesHeadOfQueue);

    RUN_TEST(test_print_emptyQueue);
    RUN_TEST(test_print_listsMessagesInPriorityOrder);
    RUN_TEST(test_print_toleratesNull);

    remove(STDOUT_CAPTURE);
    return UNITY_END();
}
