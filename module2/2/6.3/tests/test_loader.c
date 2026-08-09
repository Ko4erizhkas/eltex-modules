/*
 * Юнит-тесты для loader.c (задание 6.3)
 *
 * Загрузчик тем и интересен, что работает с настоящими файлами на диске,
 * поэтому тестировать его в отрыве от них бессмысленно. Каталог fixtures
 * собирается Makefile'ом тестов и содержит три библиотеки: две корректные
 * и одну без нужных символов - на ней проверяется, что кривая библиотека
 * пропускается, а не роняет программу.
 *
 * Весь API loader.c публичный, поэтому файл линкуется как обычно.
 * main() берётся из этого файла, main.c в сборку тестов не входит.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h> /* SetConsoleOutputCP, CP_UTF8 */
#endif

#include "unity/unity.h"
#include "../loader.h"

#define FIXTURE_DIR "fixtures"
#define PLUGIN_DIR  "../plugins"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

static OperationTable table;

/* Загружает каталог фикстур и проваливает тест, если он не собран. */
static void loadFixtures(void)
{
    int count = ops_load_dir(&table, FIXTURE_DIR);

    TEST_ASSERT_TRUE_MESSAGE(count > 0,
                             "fixtures are not built - run make in tests first");
}

/* Имя файла сравнивается по префиксу: расширение зависит от системы. */
static void assertFileStartsWith(const char* file, const char* prefix)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, strncmp(file, prefix, strlen(prefix)), file);
}

void setUp(void)
{
    table.count = 0;
}

void tearDown(void)
{
    ops_unload(&table);
}

/* ------------------------------------------------------------------ */
/* ops_load_dir - успешная загрузка                                    */
/* ------------------------------------------------------------------ */

/* В каталоге три библиотеки, но одна без нужных символов - её быть не должно */
static void test_loadDir_loadsOnlyValidLibraries(void)
{
    loadFixtures();

    TEST_ASSERT_EQUAL_INT(2, table.count);
}

static void test_loadDir_readsNameFromLibrary(void)
{
    loadFixtures();

    TEST_ASSERT_EQUAL_STRING("TEST add", table.items[0].name);
    TEST_ASSERT_EQUAL_STRING("TEST mul", table.items[1].name);
}

/* readdir порядок не гарантирует, загрузчик обязан отсортировать сам */
static void test_loadDir_sortsOperationsByFileName(void)
{
    loadFixtures();

    assertFileStartsWith(table.items[0].file, "01_add");
    assertFileStartsWith(table.items[1].file, "02_mul");
}

static void test_loadDir_keepsHandleAndFunctionPointer(void)
{
    loadFixtures();

    for (int i = 0; i < table.count; ++i)
    {
        TEST_ASSERT_NOT_NULL(table.items[i].handle);
        TEST_ASSERT_NOT_NULL(table.items[i].apply);
    }
}

/* Ради этого всё и затевалось: функция вызывается из загруженной библиотеки */
static void test_loadedFunctions_areCallable(void)
{
    loadFixtures();

    TEST_ASSERT_EQUAL_DOUBLE(5.0, table.items[0].apply(2.0, 3.0));
    TEST_ASSERT_EQUAL_DOUBLE(6.0, table.items[1].apply(2.0, 3.0));
}

static void test_loadedFunctions_workWithNegativeAndFractional(void)
{
    loadFixtures();

    TEST_ASSERT_EQUAL_DOUBLE(-1.0, table.items[0].apply(2.0, -3.0));
    TEST_ASSERT_EQUAL_DOUBLE(1.25, table.items[1].apply(0.5, 2.5));
}

/* ------------------------------------------------------------------ */
/* ops_load_dir - отказы                                               */
/* ------------------------------------------------------------------ */

static void test_loadDir_missingDirectory_reportsZero(void)
{
    TEST_ASSERT_EQUAL_INT(0, ops_load_dir(&table, "no_such_directory"));
    TEST_ASSERT_EQUAL_INT(0, table.count);
}

/* Каталог есть, библиотек в нём нет - это не ошибка, просто ноль операций */
static void test_loadDir_directoryWithoutLibraries_reportsZero(void)
{
    TEST_ASSERT_EQUAL_INT(0, ops_load_dir(&table, "unity"));
    TEST_ASSERT_EQUAL_INT(0, table.count);
}

static void test_loadDir_nullArguments_reportZero(void)
{
    TEST_ASSERT_EQUAL_INT(0, ops_load_dir(NULL, FIXTURE_DIR));
    TEST_ASSERT_EQUAL_INT(0, ops_load_dir(&table, NULL));
}

/* ------------------------------------------------------------------ */
/* ops_unload                                                          */
/* ------------------------------------------------------------------ */

static void test_unload_emptiesTable(void)
{
    loadFixtures();

    ops_unload(&table);

    TEST_ASSERT_EQUAL_INT(0, table.count);
}

static void test_unload_isSafeToCallTwice(void)
{
    loadFixtures();

    ops_unload(&table);
    ops_unload(&table);
    ops_unload(NULL);

    TEST_PASS();
}

/* Таблицу можно набрать заново после выгрузки */
static void test_loadDir_afterUnload_worksAgain(void)
{
    loadFixtures();
    ops_unload(&table);

    TEST_ASSERT_EQUAL_INT(2, ops_load_dir(&table, FIXTURE_DIR));
    TEST_ASSERT_EQUAL_DOUBLE(5.0, table.items[0].apply(2.0, 3.0));
}

/* ------------------------------------------------------------------ */
/* Настоящие библиотеки операций калькулятора                          */
/* ------------------------------------------------------------------ */

/*
 * Каталог plugins собирается Makefile'ом задания, а не тестов. Если его
 * ещё нет, тест не падает, а отмечается пропущенным - собрать программу
 * это не мешает.
 */
static void test_realPlugins_giveFourOperations(void)
{
    if (ops_load_dir(&table, PLUGIN_DIR) == 0)
    {
        TEST_IGNORE_MESSAGE("plugins are not built - run make in the task directory");
    }

    TEST_ASSERT_EQUAL_INT(4, table.count);
}

static void test_realPlugins_computeArithmetic(void)
{
    if (ops_load_dir(&table, PLUGIN_DIR) == 0)
    {
        TEST_IGNORE_MESSAGE("plugins are not built - run make in the task directory");
    }

    /* порядок задан числовыми префиксами имён файлов: + - * / */
    TEST_ASSERT_EQUAL_DOUBLE(8.0, table.items[0].apply(5.0, 3.0));
    TEST_ASSERT_EQUAL_DOUBLE(2.0, table.items[1].apply(5.0, 3.0));
    TEST_ASSERT_EQUAL_DOUBLE(15.0, table.items[2].apply(5.0, 3.0));
    TEST_ASSERT_EQUAL_DOUBLE(2.5, table.items[3].apply(5.0, 2.0));
}

static void test_realPlugins_haveNonEmptyNames(void)
{
    if (ops_load_dir(&table, PLUGIN_DIR) == 0)
    {
        TEST_IGNORE_MESSAGE("plugins are not built - run make in the task directory");
    }

    for (int i = 0; i < table.count; ++i)
    {
        TEST_ASSERT_TRUE_MESSAGE(table.items[i].name[0] != '\0',
                                 "every operation is expected to have a name");
    }
}

/* ------------------------------------------------------------------ */

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    UNITY_BEGIN();

    RUN_TEST(test_loadDir_loadsOnlyValidLibraries);
    RUN_TEST(test_loadDir_readsNameFromLibrary);
    RUN_TEST(test_loadDir_sortsOperationsByFileName);
    RUN_TEST(test_loadDir_keepsHandleAndFunctionPointer);

    RUN_TEST(test_loadedFunctions_areCallable);
    RUN_TEST(test_loadedFunctions_workWithNegativeAndFractional);

    RUN_TEST(test_loadDir_missingDirectory_reportsZero);
    RUN_TEST(test_loadDir_directoryWithoutLibraries_reportsZero);
    RUN_TEST(test_loadDir_nullArguments_reportZero);

    RUN_TEST(test_unload_emptiesTable);
    RUN_TEST(test_unload_isSafeToCallTwice);
    RUN_TEST(test_loadDir_afterUnload_worksAgain);

    RUN_TEST(test_realPlugins_giveFourOperations);
    RUN_TEST(test_realPlugins_computeArithmetic);
    RUN_TEST(test_realPlugins_haveNonEmptyNames);

    return UNITY_END();
}
