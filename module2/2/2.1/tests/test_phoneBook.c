/*
 * Юнит-тесты для phoneBook.c
 *
 * Все функции в phoneBook.c объявлены static, поэтому слинковать их
 * из отдельного объектного файла нельзя. Стандартный приём для такого
 * случая - включить сам .c-файл в тест: тогда тест и тестируемый код
 * попадают в одну единицу трансляции и static перестаёт мешать.
 * main() берётся из этого файла, main.c в сборку тестов не входит.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <windows.h> /* SetConsoleOutputCP, CP_UTF8 */

#include "unity/unity.h"
#include "../phoneBook.c"

#define STDIN_FIXTURE "stdin_fixture.tmp"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

/*
 * Тестируемые функции печатают приглашения ввода в stdout.
 * Глушим его на время теста, чтобы отчёт Unity оставался читаемым.
 * Unity печатает свои строки уже после tearDown, поэтому её вывод не страдает.
 */
static int savedStdoutFd = -1;

static void muteStdout(void)
{
    fflush(stdout);
    savedStdoutFd = _dup(_fileno(stdout));
    freopen("NUL", "w", stdout);
}

static void unmuteStdout(void)
{
    if (savedStdoutFd < 0) return;
    fflush(stdout);
    _dup2(savedStdoutFd, _fileno(stdout));
    _close(savedStdoutFd);
    savedStdoutFd = -1;
    clearerr(stdout);
}

/* Подменяет stdin содержимым строки - так тестируются функции с fgets. */
static void feedStdin(const char* input)
{
    FILE* f = fopen(STDIN_FIXTURE, "wb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "не удалось создать файл-фикстуру для stdin");
    fputs(input, f);
    fclose(f);

    TEST_ASSERT_NOT_NULL_MESSAGE(freopen(STDIN_FIXTURE, "r", stdin),
                                 "не удалось перенаправить stdin");
}

/* Контакт с заданным числом номеров и соцсетей, поля вида "num0", "tg0"... */
static Contact* makeContact(const char* surname, size_t numbers, size_t socials)
{
    Contact* c = calloc(1, sizeof(Contact));
    TEST_ASSERT_NOT_NULL(c);

    strcpy(c->surname, surname);
    strcpy(c->name, "Name");
    strcpy(c->patronymic, "Patronymic");

    c->countNumbers = numbers;
    for (size_t i = 0; i < numbers; ++i)
    {
        snprintf(c->phonenumber[i].number, MAX_SIZE, "num%zu", i);
    }

    c->countSocial = socials;
    for (size_t i = 0; i < socials; ++i)
    {
        snprintf(c->social[i].typeSocial, MAX_SIZE, "type%zu", i);
        snprintf(c->social[i].username, MAX_SIZE, "user%zu", i);
        snprintf(c->social[i].link, MAX_SIZE_LINK, "link%zu", i);
    }

    return c;
}

static Phonebook* makePhonebook(void)
{
    Phonebook* pb = calloc(1, sizeof(Phonebook));
    TEST_ASSERT_NOT_NULL(pb);
    pb->countContacts = 0;
    return pb;
}

void setUp(void)
{
    muteStdout();
}

void tearDown(void)
{
    unmuteStdout();
}

/* ------------------------------------------------------------------ */
/* addContactInPhonebook                                               */
/* ------------------------------------------------------------------ */

static void test_addContactInPhonebook_incrementsCount(void)
{
    Phonebook* pb = makePhonebook();
    Contact* c = makeContact("Ivanov", 0, 0);

    addContactInPhonebook(pb, c);

    TEST_ASSERT_EQUAL_INT(1, pb->countContacts);

    free(c);
    free(pb);
}

static void test_addContactInPhonebook_storesPointerAtTail(void)
{
    Phonebook* pb = makePhonebook();
    Contact* first = makeContact("Ivanov", 0, 0);
    Contact* second = makeContact("Petrov", 0, 0);

    addContactInPhonebook(pb, first);
    addContactInPhonebook(pb, second);

    TEST_ASSERT_EQUAL_PTR(first, pb->contacts[0]);
    TEST_ASSERT_EQUAL_PTR(second, pb->contacts[1]);
    TEST_ASSERT_EQUAL_INT(2, pb->countContacts);

    free(first);
    free(second);
    free(pb);
}

static void test_addContactInPhonebook_rejectsWhenFull(void)
{
    /* Сообщение латиницей: Unity экранирует любой байт вне ASCII как \xNN. */
    TEST_IGNORE_MESSAGE(
        "addContactInPhonebook does not check countContacts < "
        "MAX_COUNT_CONTACTS and writes past the array when the phonebook is "
        "full. Running this test would corrupt memory.");
}

/* ------------------------------------------------------------------ */
/* deleteContact                                                       */
/* ------------------------------------------------------------------ */

static void test_deleteContact_first_shiftsRestLeft(void)
{
    Phonebook* pb = makePhonebook();
    Contact* a = makeContact("A", 0, 0);
    Contact* b = makeContact("B", 0, 0);
    Contact* c = makeContact("C", 0, 0);

    addContactInPhonebook(pb, a);
    addContactInPhonebook(pb, b);
    addContactInPhonebook(pb, c);

    deleteContact(pb, 0); /* освобождает a */

    TEST_ASSERT_EQUAL_INT(2, pb->countContacts);
    TEST_ASSERT_EQUAL_PTR(b, pb->contacts[0]);
    TEST_ASSERT_EQUAL_PTR(c, pb->contacts[1]);
    TEST_ASSERT_NULL(pb->contacts[2]);

    free(b);
    free(c);
    free(pb);
}

static void test_deleteContact_last_clearsSlot(void)
{
    Phonebook* pb = makePhonebook();
    Contact* a = makeContact("A", 0, 0);
    Contact* b = makeContact("B", 0, 0);

    addContactInPhonebook(pb, a);
    addContactInPhonebook(pb, b);

    deleteContact(pb, 1); /* освобождает b */

    TEST_ASSERT_EQUAL_INT(1, pb->countContacts);
    TEST_ASSERT_EQUAL_PTR(a, pb->contacts[0]);
    TEST_ASSERT_NULL(pb->contacts[1]);

    free(a);
    free(pb);
}

static void test_deleteContact_rejectsBadIndex(void)
{
    TEST_IGNORE_MESSAGE(
        "deleteContact does not validate index: for index < 0 or index >= "
        "countContacts it calls free() on a garbage pointer and the memmove "
        "size underflows. Cannot be asserted without crashing the runner.");
}

/* ------------------------------------------------------------------ */
/* deleteNumber                                                        */
/* ------------------------------------------------------------------ */

static void test_deleteNumber_middle_shiftsRestLeft(void)
{
    Contact* c = makeContact("Ivanov", 3, 0);

    deleteNumber(c, 1);

    TEST_ASSERT_EQUAL_UINT(2, (unsigned)c->countNumbers);
    TEST_ASSERT_EQUAL_STRING("num0", c->phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("num2", c->phonenumber[1].number);
    TEST_ASSERT_EQUAL_STRING("", c->phonenumber[2].number);

    free(c);
}

static void test_deleteNumber_last_clearsSlot(void)
{
    Contact* c = makeContact("Ivanov", 2, 0);

    deleteNumber(c, 1);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)c->countNumbers);
    TEST_ASSERT_EQUAL_STRING("num0", c->phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("", c->phonenumber[1].number);

    free(c);
}

static void test_deleteNumber_negativeIndex_changesNothing(void)
{
    Contact* c = makeContact("Ivanov", 2, 0);

    deleteNumber(c, -1);

    TEST_ASSERT_EQUAL_UINT(2, (unsigned)c->countNumbers);
    TEST_ASSERT_EQUAL_STRING("num0", c->phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("num1", c->phonenumber[1].number);

    free(c);
}

static void test_deleteNumber_indexEqualToCount_rejected(void)
{
    TEST_IGNORE_MESSAGE(
        "Bound is checked as index > countNumbers, but the last valid index "
        "is countNumbers - 1. At index == countNumbers the expression "
        "(countNumbers - index - 1) underflows size_t to SIZE_MAX and "
        "memmove crashes the process. Same bug in deleteSocial/editNumber/"
        "editSocial.");
}

/* ------------------------------------------------------------------ */
/* deleteSocial                                                        */
/* ------------------------------------------------------------------ */

static void test_deleteSocial_middle_shiftsRestLeft(void)
{
    Contact* c = makeContact("Ivanov", 1, 3);

    deleteSocial(c, 1);

    TEST_ASSERT_EQUAL_UINT(2, (unsigned)c->countSocial);
    TEST_ASSERT_EQUAL_STRING("type0", c->social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("type2", c->social[1].typeSocial);
    TEST_ASSERT_EQUAL_STRING("user2", c->social[1].username);
    TEST_ASSERT_EQUAL_STRING("link2", c->social[1].link);

    free(c);
}

/*
 * Ключевой тест: после удаления должен очищаться освободившийся хвостовой
 * слот - social[countSocial - 1]. Контакт намеренно создан так, что
 * countNumbers (1) не совпадает с countSocial (3).
 */
static void test_deleteSocial_clearsTailSlotOnly(void)
{
    Contact* c = makeContact("Ivanov", 1, 3);

    deleteSocial(c, 0);

    /* Живые данные, съехавшие на место удалённого, должны уцелеть. */
    TEST_ASSERT_EQUAL_STRING("type1", c->social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("user1", c->social[0].username);

    /* Хвостовой слот должен быть очищен целиком. */
    TEST_ASSERT_EQUAL_STRING("", c->social[2].typeSocial);
    TEST_ASSERT_EQUAL_STRING("", c->social[2].username);
    TEST_ASSERT_EQUAL_STRING("", c->social[2].link);

    free(c);
}

static void test_deleteSocial_negativeIndex_changesNothing(void)
{
    Contact* c = makeContact("Ivanov", 0, 2);

    deleteSocial(c, -1);

    TEST_ASSERT_EQUAL_UINT(2, (unsigned)c->countSocial);
    TEST_ASSERT_EQUAL_STRING("type0", c->social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("type1", c->social[1].typeSocial);

    free(c);
}

/* ------------------------------------------------------------------ */
/* addNames                                                            */
/* ------------------------------------------------------------------ */

static void test_addNames_readsSurnameNamePatronymic(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("Ivanov\nIvan\nIvanovich\n");

    addNames(c);

    TEST_ASSERT_EQUAL_STRING("Ivanov", c->surname);
    TEST_ASSERT_EQUAL_STRING("Ivan", c->name);
    TEST_ASSERT_EQUAL_STRING("Ivanovich", c->patronymic);

    free(c);
}

static void test_addNames_emptyPatronymic_becomesDash(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("Ivanov\nIvan\n\n");

    addNames(c);

    TEST_ASSERT_EQUAL_STRING("-", c->patronymic);

    free(c);
}

static void test_addNames_emptySurname_isAskedAgain(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("\nIvanov\nIvan\nIvanovich\n");

    addNames(c);

    TEST_ASSERT_EQUAL_STRING("Ivanov", c->surname);
    TEST_ASSERT_EQUAL_STRING("Ivan", c->name);

    free(c);
}

/* ------------------------------------------------------------------ */
/* addNumber                                                           */
/* ------------------------------------------------------------------ */

static void test_addNumber_stopsOnEmptyLine(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("+7-900-111\n+7-900-222\n\n");

    addNumber(c);

    TEST_ASSERT_EQUAL_UINT(2, (unsigned)c->countNumbers);
    TEST_ASSERT_EQUAL_STRING("+7-900-111", c->phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("+7-900-222", c->phonenumber[1].number);

    free(c);
}

static void test_addNumber_stopsAtMaxCount(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("n1\nn2\nn3\nn4\nn5\n");

    addNumber(c);

    TEST_ASSERT_EQUAL_UINT(MAX_COUNT, (unsigned)c->countNumbers);
    TEST_ASSERT_EQUAL_STRING("n4", c->phonenumber[MAX_COUNT - 1].number);

    free(c);
}

static void test_addNumber_stopsOnEof(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("n1\n");

    addNumber(c);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)c->countNumbers);

    free(c);
}

static void test_addNumber_nullContact_doesNotCrash(void)
{
    addNumber(NULL);
    TEST_PASS();
}

/* ------------------------------------------------------------------ */
/* addSocial                                                           */
/* ------------------------------------------------------------------ */

static void test_addSocial_readsOneEntry(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("Telegram\nivanov\nhttps://t.me/ivanov\n\n");

    addSocial(c);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)c->countSocial);
    TEST_ASSERT_EQUAL_STRING("Telegram", c->social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("ivanov", c->social[0].username);
    TEST_ASSERT_EQUAL_STRING("https://t.me/ivanov", c->social[0].link);

    free(c);
}

static void test_addSocial_emptyPlatform_stopsImmediately(void)
{
    Contact* c = calloc(1, sizeof(Contact));
    feedStdin("\n");

    addSocial(c);

    TEST_ASSERT_EQUAL_UINT(0, (unsigned)c->countSocial);

    free(c);
}

/* ------------------------------------------------------------------ */
/* editNames                                                           */
/* ------------------------------------------------------------------ */

static void test_editNames_emptyInput_keepsOldValues(void)
{
    Contact* c = makeContact("Ivanov", 0, 0);
    feedStdin("\n\n\n");

    editNames(c);

    TEST_ASSERT_EQUAL_STRING("Ivanov", c->surname);
    TEST_ASSERT_EQUAL_STRING("Name", c->name);
    TEST_ASSERT_EQUAL_STRING("Patronymic", c->patronymic);

    free(c);
}

/* Порядок ввода в editNames: имя, фамилия, отчество. */
static void test_editNames_replacesAllFields(void)
{
    Contact* c = makeContact("Ivanov", 0, 0);
    feedStdin("Petr\nPetrov\nPetrovich\n");

    editNames(c);

    TEST_ASSERT_EQUAL_STRING("Petr", c->name);
    TEST_ASSERT_EQUAL_STRING("Petrov", c->surname);
    TEST_ASSERT_EQUAL_STRING("Petrovich", c->patronymic);

    free(c);
}

/* ------------------------------------------------------------------ */
/* editSocial                                                          */
/* ------------------------------------------------------------------ */

/* Порядок ввода в editSocial: никнейм, тип соцсети, ссылка. */
static void test_editSocial_replacesFields(void)
{
    Contact* c = makeContact("Ivanov", 0, 2);
    feedStdin("newuser\nVK\nhttps://vk.com/newuser\n");

    editSocial(c, 0);

    TEST_ASSERT_EQUAL_STRING("newuser", c->social[0].username);
    TEST_ASSERT_EQUAL_STRING("VK", c->social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("https://vk.com/newuser", c->social[0].link);
    /* Соседний элемент трогать не должны. */
    TEST_ASSERT_EQUAL_STRING("type1", c->social[1].typeSocial);

    free(c);
}

static void test_editSocial_emptyInput_keepsOldValues(void)
{
    Contact* c = makeContact("Ivanov", 0, 1);
    feedStdin("\n\n\n");

    editSocial(c, 0);

    TEST_ASSERT_EQUAL_STRING("user0", c->social[0].username);
    TEST_ASSERT_EQUAL_STRING("type0", c->social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("link0", c->social[0].link);

    free(c);
}

static void test_editSocial_negativeIndex_changesNothing(void)
{
    Contact* c = makeContact("Ivanov", 0, 1);
    feedStdin("hacked\nhacked\nhacked\n");

    editSocial(c, -1);

    TEST_ASSERT_EQUAL_STRING("user0", c->social[0].username);

    free(c);
}

/* ------------------------------------------------------------------ */
/* editNumber                                                          */
/* ------------------------------------------------------------------ */

static void test_editNumber_replacesNumber(void)
{
    TEST_IGNORE_MESSAGE(
        "editNumber prints c->phonenumber[index] with %s - a Numbers struct "
        "is passed where printf expects char*. Undefined behaviour, almost "
        "certainly a crash, so the function is not called here. See "
        "phoneBook.c:241.");
}

/* ------------------------------------------------------------------ */
/* addContact - сценарий целиком                                       */
/* ------------------------------------------------------------------ */

static void test_addContact_buildsCompleteContact(void)
{
    feedStdin("Petrov\nPetr\nPetrovich\n"   /* addNames  */
              "+7-900-333\n\n"              /* addNumber */
              "Telegram\npetr\nhttps://t.me/petr\n\n"); /* addSocial */

    Contact* c = addContact();

    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_STRING("Petrov", c->surname);
    TEST_ASSERT_EQUAL_STRING("Petr", c->name);
    TEST_ASSERT_EQUAL_STRING("Petrovich", c->patronymic);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)c->countNumbers);
    TEST_ASSERT_EQUAL_STRING("+7-900-333", c->phonenumber[0].number);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)c->countSocial);
    TEST_ASSERT_EQUAL_STRING("Telegram", c->social[0].typeSocial);

    free(c);
}

/* ------------------------------------------------------------------ */
/* fillTestData                                                        */
/* ------------------------------------------------------------------ */

static void test_fillTestData_fillsPhonebookToCapacity(void)
{
    Phonebook* pb = makePhonebook();

    fillTestData(pb);

    TEST_ASSERT_EQUAL_INT(MAX_COUNT_CONTACTS, pb->countContacts);
    for (int i = 0; i < MAX_COUNT_CONTACTS; ++i)
    {
        TEST_ASSERT_NOT_NULL(pb->contacts[i]);
        TEST_ASSERT_EQUAL_UINT(MAX_COUNT, (unsigned)pb->contacts[i]->countNumbers);
        TEST_ASSERT_EQUAL_UINT(MAX_COUNT, (unsigned)pb->contacts[i]->countSocial);
        free(pb->contacts[i]);
    }

    free(pb);
}

/* ------------------------------------------------------------------ */

int main(void)
{

    SetConsoleOutputCP(CP_UTF8);

    UNITY_BEGIN();

    RUN_TEST(test_addContactInPhonebook_incrementsCount);
    RUN_TEST(test_addContactInPhonebook_storesPointerAtTail);
    RUN_TEST(test_addContactInPhonebook_rejectsWhenFull);

    RUN_TEST(test_deleteContact_first_shiftsRestLeft);
    RUN_TEST(test_deleteContact_last_clearsSlot);
    RUN_TEST(test_deleteContact_rejectsBadIndex);

    RUN_TEST(test_deleteNumber_middle_shiftsRestLeft);
    RUN_TEST(test_deleteNumber_last_clearsSlot);
    RUN_TEST(test_deleteNumber_negativeIndex_changesNothing);
    RUN_TEST(test_deleteNumber_indexEqualToCount_rejected);

    RUN_TEST(test_deleteSocial_middle_shiftsRestLeft);
    RUN_TEST(test_deleteSocial_clearsTailSlotOnly);
    RUN_TEST(test_deleteSocial_negativeIndex_changesNothing);

    RUN_TEST(test_addNames_readsSurnameNamePatronymic);
    RUN_TEST(test_addNames_emptyPatronymic_becomesDash);
    RUN_TEST(test_addNames_emptySurname_isAskedAgain);

    RUN_TEST(test_addNumber_stopsOnEmptyLine);
    RUN_TEST(test_addNumber_stopsAtMaxCount);
    RUN_TEST(test_addNumber_stopsOnEof);
    RUN_TEST(test_addNumber_nullContact_doesNotCrash);

    RUN_TEST(test_addSocial_readsOneEntry);
    RUN_TEST(test_addSocial_emptyPlatform_stopsImmediately);

    RUN_TEST(test_editNames_emptyInput_keepsOldValues);
    RUN_TEST(test_editNames_replacesAllFields);

    RUN_TEST(test_editSocial_replacesFields);
    RUN_TEST(test_editSocial_emptyInput_keepsOldValues);
    RUN_TEST(test_editSocial_negativeIndex_changesNothing);

    RUN_TEST(test_editNumber_replacesNumber);

    RUN_TEST(test_addContact_buildsCompleteContact);
    RUN_TEST(test_fillTestData_fillsPhonebookToCapacity);

    remove(STDIN_FIXTURE);
    return UNITY_END();
}
