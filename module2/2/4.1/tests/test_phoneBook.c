/*
 * Юнит-тесты для phoneBook.c (задание 4.1)
 *
 * Основное отличие от 2.1 - хранение на двусвязном упорядоченном списке,
 * поэтому большая часть тестов проверяет именно его: инварианты цепочки,
 * порядок узлов и четыре крайних случая вставки и удаления.
 *
 * Часть функций в phoneBook.c объявлена static, слинковать их из отдельного
 * объектного файла нельзя. Стандартный приём - включить сам .c-файл в тест:
 * тест и тестируемый код попадают в одну единицу трансляции, и static
 * перестаёт мешать. main() берётся из этого файла, main.c в сборку не входит.
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

/* Фамилии латиницей: Unity экранирует любой байт вне ASCII как \xNN, и
   сообщение об упавшем сравнении иначе невозможно прочитать. */
static Contact makeContact(const char* surname, const char* name,
                           const char* patronymic)
{
    Contact c;
    memset(&c, 0, sizeof(c));

    strcpy(c.surname, surname);
    strcpy(c.name, name);
    strcpy(c.patronymic, patronymic);

    return c;
}

static void fillFields(Contact* c, int numbers, int socials)
{
    c->countNumbers = numbers;
    for (int i = 0; i < numbers; ++i)
    {
        snprintf(c->phonenumber[i].number, MAX_SIZE, "num%d", i);
    }

    c->countSocial = socials;
    for (int i = 0; i < socials; ++i)
    {
        snprintf(c->social[i].typeSocial, MAX_SIZE, "type%d", i);
        snprintf(c->social[i].username, MAX_SIZE, "user%d", i);
        snprintf(c->social[i].link, MAX_SIZE_LINK, "link%d", i);
    }
}

static PHNode* push(PhonebookList* pb, const char* surname)
{
    Contact c = makeContact(surname, "Name", "Patronymic");
    return phlist_insert(pb, &c);
}

/* Фамилии в порядке обхода от головы к хвосту. */
static const char* joinForward(const PhonebookList* pb)
{
    static char buffer[512];
    buffer[0] = '\0';

    for (const PHNode* n = pb->head; n != NULL; n = n->next)
    {
        if (buffer[0] != '\0') strcat(buffer, ",");
        strcat(buffer, n->data.surname);
    }

    return buffer;
}

/* То же самое от хвоста к голове - проверяет обратные ссылки prev. */
static const char* joinBackward(const PhonebookList* pb)
{
    static char buffer[512];
    buffer[0] = '\0';

    for (const PHNode* n = pb->tail; n != NULL; n = n->prev)
    {
        if (buffer[0] != '\0') strcat(buffer, ",");
        strcat(buffer, n->data.surname);
    }

    return buffer;
}

/*
 * Полная проверка инвариантов из README: концы цепочки, взаимность prev/next,
 * согласованность size с реальной длиной и неубывание порядка.
 */
static void assertListValid(const PhonebookList* pb)
{
    TEST_ASSERT_NOT_NULL(pb);

    if (pb->head == NULL)
    {
        TEST_ASSERT_NULL_MESSAGE(pb->tail, "empty list must have no tail");
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, (unsigned)pb->size,
                                       "empty list must have size 0");
        return;
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(pb->tail, "non-empty list must have a tail");
    TEST_ASSERT_NULL_MESSAGE(pb->head->prev, "head->prev must be NULL");
    TEST_ASSERT_NULL_MESSAGE(pb->tail->next, "tail->next must be NULL");

    unsigned count = 0;
    for (const PHNode* n = pb->head; n != NULL; n = n->next)
    {
        count++;

        if (n->next != NULL)
        {
            TEST_ASSERT_EQUAL_PTR_MESSAGE(n, n->next->prev,
                                          "node->next->prev must point back");
            TEST_ASSERT_TRUE_MESSAGE(contact_cmp(&n->data, &n->next->data) <= 0,
                                     "list order is broken");
        }
        else
        {
            TEST_ASSERT_EQUAL_PTR_MESSAGE(pb->tail, n,
                                          "last node must be the tail");
        }
    }

    TEST_ASSERT_EQUAL_UINT_MESSAGE((unsigned)pb->size, count,
                                   "size does not match the real length");
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
/* contact_cmp                                                         */
/* ------------------------------------------------------------------ */

static void test_contactCmp_identicalContacts_giveZero(void)
{
    Contact a = makeContact("Ivanov", "Ivan", "Ivanovich");
    Contact b = makeContact("Ivanov", "Ivan", "Ivanovich");

    TEST_ASSERT_EQUAL_INT(0, contact_cmp(&a, &b));
}

static void test_contactCmp_surnameDecidesFirst(void)
{
    Contact a = makeContact("Alpha", "Zzz", "Zzz");
    Contact b = makeContact("Beta", "Aaa", "Aaa");

    TEST_ASSERT_TRUE_MESSAGE(contact_cmp(&a, &b) < 0,
                             "surname must outweigh name and patronymic");
    TEST_ASSERT_TRUE_MESSAGE(contact_cmp(&b, &a) > 0,
                             "comparison must be antisymmetric");
}

static void test_contactCmp_nameDecidesWhenSurnamesMatch(void)
{
    Contact a = makeContact("Ivanov", "Alpha", "Zzz");
    Contact b = makeContact("Ivanov", "Beta", "Aaa");

    TEST_ASSERT_TRUE_MESSAGE(contact_cmp(&a, &b) < 0,
                             "name must decide when surnames are equal");
}

static void test_contactCmp_patronymicDecidesLast(void)
{
    Contact a = makeContact("Ivanov", "Ivan", "Alpha");
    Contact b = makeContact("Ivanov", "Ivan", "Beta");

    TEST_ASSERT_TRUE_MESSAGE(contact_cmp(&a, &b) < 0,
                             "patronymic must decide when the rest is equal");
}

/* ------------------------------------------------------------------ */
/* phlist_init / phlist_free                                           */
/* ------------------------------------------------------------------ */

static void test_init_returnsEmptyList(void)
{
    PhonebookList* pb = phlist_init();

    TEST_ASSERT_NOT_NULL(pb);
    TEST_ASSERT_NULL(pb->head);
    TEST_ASSERT_NULL(pb->tail);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->size);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_free_nullsCallersPointer(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");

    phlist_free(&pb);

    TEST_ASSERT_NULL_MESSAGE(pb, "phlist_free must null the caller's pointer");
}

static void test_free_onEmptyList_doesNotCrash(void)
{
    PhonebookList* pb = phlist_init();

    phlist_free(&pb);

    TEST_ASSERT_NULL(pb);
}

static void test_free_nullArgument_doesNotCrash(void)
{
    PhonebookList* pb = NULL;

    phlist_free(NULL);
    phlist_free(&pb);

    TEST_ASSERT_NULL(pb);
}

/* ------------------------------------------------------------------ */
/* phlist_insert - крайние случаи                                      */
/* ------------------------------------------------------------------ */

static void test_insert_intoEmptyList_becomesHeadAndTail(void)
{
    PhonebookList* pb = phlist_init();

    PHNode* node = push(pb, "Alpha");

    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_PTR(node, pb->head);
    TEST_ASSERT_EQUAL_PTR(node, pb->tail);
    TEST_ASSERT_NULL(node->prev);
    TEST_ASSERT_NULL(node->next);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_insert_smallest_goesToHead(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Beta");
    push(pb, "Gamma");

    PHNode* node = push(pb, "Alpha");

    TEST_ASSERT_EQUAL_PTR_MESSAGE(node, pb->head,
                                  "smallest contact must become the head");
    TEST_ASSERT_EQUAL_STRING("Alpha,Beta,Gamma", joinForward(pb));
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_insert_largest_goesToTail(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");

    PHNode* node = push(pb, "Gamma");

    TEST_ASSERT_EQUAL_PTR_MESSAGE(node, pb->tail,
                                  "largest contact must become the tail");
    TEST_ASSERT_EQUAL_STRING("Alpha,Beta,Gamma", joinForward(pb));
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_insert_middle_linksBothNeighbours(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Gamma");

    PHNode* node = push(pb, "Beta");

    TEST_ASSERT_EQUAL_PTR(pb->head, node->prev);
    TEST_ASSERT_EQUAL_PTR(pb->tail, node->next);
    TEST_ASSERT_EQUAL_PTR(node, pb->head->next);
    TEST_ASSERT_EQUAL_PTR(node, pb->tail->prev);
    assertListValid(pb);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phlist_insert - порядок                                             */
/* ------------------------------------------------------------------ */

static void test_insert_keepsOrderRegardlessOfInsertionOrder(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "Delta");
    push(pb, "Alpha");
    push(pb, "Gamma");
    push(pb, "Beta");

    /* порядок лексикографический, а не греческого алфавита: Delta < Gamma */
    TEST_ASSERT_EQUAL_STRING("Alpha,Beta,Delta,Gamma", joinForward(pb));
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_insert_backwardTraversalMatchesForward(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "Delta");
    push(pb, "Alpha");
    push(pb, "Gamma");
    push(pb, "Beta");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("Gamma,Delta,Beta,Alpha",
                                     joinBackward(pb),
                                     "prev chain must mirror the next chain");

    phlist_free(&pb);
}

static void test_insert_ordersByNameWhenSurnamesMatch(void)
{
    PhonebookList* pb = phlist_init();

    Contact second = makeContact("Ivanov", "Boris", "Petrovich");
    Contact first = makeContact("Ivanov", "Anton", "Petrovich");

    phlist_insert(pb, &second);
    phlist_insert(pb, &first);

    TEST_ASSERT_EQUAL_STRING("Anton", pb->head->data.name);
    TEST_ASSERT_EQUAL_STRING("Boris", pb->tail->data.name);
    assertListValid(pb);

    phlist_free(&pb);
}

/* Равные Ф.И.О. встают в конец своей группы - порядок добавления сохраняется. */
static void test_insert_equalContacts_keepInsertionOrder(void)
{
    PhonebookList* pb = phlist_init();

    Contact first = makeContact("Ivanov", "Ivan", "Ivanovich");
    strcpy(first.phonenumber[0].number, "first");
    first.countNumbers = 1;

    Contact second = makeContact("Ivanov", "Ivan", "Ivanovich");
    strcpy(second.phonenumber[0].number, "second");
    second.countNumbers = 1;

    phlist_insert(pb, &first);
    phlist_insert(pb, &second);

    TEST_ASSERT_EQUAL_STRING("first", pb->head->data.phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("second", pb->tail->data.phonenumber[0].number);
    assertListValid(pb);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phlist_insert - владение данными                                    */
/* ------------------------------------------------------------------ */

static void test_insert_copiesContactByValue(void)
{
    PhonebookList* pb = phlist_init();

    Contact source = makeContact("Alpha", "Name", "Patronymic");
    PHNode* node = phlist_insert(pb, &source);

    strcpy(source.surname, "Changed");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("Alpha", node->data.surname,
                                     "node must hold a copy, not a reference");

    phlist_free(&pb);
}

static void test_insert_nullArguments_returnNull(void)
{
    PhonebookList* pb = phlist_init();
    Contact c = makeContact("Alpha", "Name", "Patronymic");

    TEST_ASSERT_NULL(phlist_insert(NULL, &c));
    TEST_ASSERT_NULL(phlist_insert(pb, NULL));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, (unsigned)pb->size,
                                   "rejected insert must not change size");

    phlist_free(&pb);
}

static void test_insert_incrementsSize(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "Alpha");
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);

    push(pb, "Beta");
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)pb->size);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phlist_at                                                           */
/* ------------------------------------------------------------------ */

static void test_at_returnsNodesInSortedOrder(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Gamma");
    push(pb, "Alpha");
    push(pb, "Beta");

    TEST_ASSERT_EQUAL_STRING("Alpha", phlist_at(pb, 0)->data.surname);
    TEST_ASSERT_EQUAL_STRING("Beta", phlist_at(pb, 1)->data.surname);
    TEST_ASSERT_EQUAL_STRING("Gamma", phlist_at(pb, 2)->data.surname);

    phlist_free(&pb);
}

static void test_at_firstAndLastMatchHeadAndTail(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    TEST_ASSERT_EQUAL_PTR(pb->head, phlist_at(pb, 0));
    TEST_ASSERT_EQUAL_PTR(pb->tail, phlist_at(pb, pb->size - 1));

    phlist_free(&pb);
}

static void test_at_indexEqualToSize_returnsNull(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");

    TEST_ASSERT_NULL_MESSAGE(phlist_at(pb, 2),
                             "index equal to size is out of range");

    phlist_free(&pb);
}

static void test_at_onEmptyList_returnsNull(void)
{
    PhonebookList* pb = phlist_init();

    TEST_ASSERT_NULL(phlist_at(pb, 0));

    phlist_free(&pb);
}

static void test_at_nullList_returnsNull(void)
{
    TEST_ASSERT_NULL(phlist_at(NULL, 0));
}

/* ------------------------------------------------------------------ */
/* phlist_erase - крайние случаи                                       */
/* ------------------------------------------------------------------ */

static void test_erase_head_movesHeadForward(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    phlist_erase(pb, pb->head);

    TEST_ASSERT_EQUAL_STRING("Beta,Gamma", joinForward(pb));
    TEST_ASSERT_NULL_MESSAGE(pb->head->prev, "new head must have no prev");
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_erase_tail_movesTailBackward(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    phlist_erase(pb, pb->tail);

    TEST_ASSERT_EQUAL_STRING("Alpha,Beta", joinForward(pb));
    TEST_ASSERT_NULL_MESSAGE(pb->tail->next, "new tail must have no next");
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_erase_middle_linksNeighbours(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    phlist_erase(pb, phlist_at(pb, 1));

    TEST_ASSERT_EQUAL_STRING("Alpha,Gamma", joinForward(pb));
    TEST_ASSERT_EQUAL_PTR(pb->tail, pb->head->next);
    TEST_ASSERT_EQUAL_PTR(pb->head, pb->tail->prev);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_erase_onlyNode_emptiesList(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");

    phlist_erase(pb, pb->head);

    TEST_ASSERT_NULL_MESSAGE(pb->head, "head must be NULL after last erase");
    TEST_ASSERT_NULL_MESSAGE(pb->tail, "tail must be NULL after last erase");
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->size);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_erase_everything_leavesUsableList(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    while (pb->head != NULL)
    {
        phlist_erase(pb, pb->head);
        assertListValid(pb);
    }

    push(pb, "Delta");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("Delta", joinForward(pb),
                                     "list must stay usable after full erase");
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_erase_decrementsSize(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");

    phlist_erase(pb, pb->head);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);

    phlist_free(&pb);
}

static void test_erase_nullArguments_doNotCrash(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");

    phlist_erase(pb, NULL);
    phlist_erase(NULL, pb->head);

    TEST_ASSERT_EQUAL_UINT_MESSAGE(1, (unsigned)pb->size,
                                   "rejected erase must not change size");
    assertListValid(pb);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phlist_reposition                                                   */
/* ------------------------------------------------------------------ */

static void test_reposition_renamedToSmallest_movesToHead(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    PHNode* node = phlist_at(pb, 2);
    strcpy(node->data.surname, "Aaa");
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_STRING("Aaa,Alpha,Beta", joinForward(pb));
    TEST_ASSERT_EQUAL_PTR(node, pb->head);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_reposition_renamedToLargest_movesToTail(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    PHNode* node = pb->head;
    strcpy(node->data.surname, "Zzz");
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_STRING("Beta,Gamma,Zzz", joinForward(pb));
    TEST_ASSERT_EQUAL_PTR(node, pb->tail);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_reposition_renamedToMiddle_landsBetween(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    PHNode* node = pb->head;
    strcpy(node->data.surname, "Bzz");
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_STRING("Beta,Bzz,Gamma", joinForward(pb));
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_reposition_reusesTheSameNode(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");

    PHNode* node = pb->head;
    strcpy(node->data.surname, "Zzz");
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_PTR_MESSAGE(node, pb->tail,
                                  "reposition must not reallocate the node");

    phlist_free(&pb);
}

static void test_reposition_keepsSize(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    phlist_reposition(pb, pb->head);

    TEST_ASSERT_EQUAL_UINT_MESSAGE(3, (unsigned)pb->size,
                                   "reposition must not change size");

    phlist_free(&pb);
}

static void test_reposition_unchangedKey_keepsPosition(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");
    push(pb, "Beta");
    push(pb, "Gamma");

    phlist_reposition(pb, phlist_at(pb, 1));

    TEST_ASSERT_EQUAL_STRING("Alpha,Beta,Gamma", joinForward(pb));
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_reposition_singleNode_staysHeadAndTail(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");

    PHNode* node = pb->head;
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_PTR(node, pb->head);
    TEST_ASSERT_EQUAL_PTR(node, pb->tail);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_reposition_nullArguments_doNotCrash(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Alpha");

    phlist_reposition(pb, NULL);
    phlist_reposition(NULL, pb->head);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);
    assertListValid(pb);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* fillTestData                                                        */
/* ------------------------------------------------------------------ */

static void test_fillTestData_addsAllTestContacts(void)
{
    PhonebookList* pb = phlist_init();

    fillTestData(pb);

    TEST_ASSERT_EQUAL_UINT(MAX_COUNT_CONTACTS, (unsigned)pb->size);
    assertListValid(pb);

    phlist_free(&pb);
}

static void test_fillTestData_producesSortedList(void)
{
    PhonebookList* pb = phlist_init();

    fillTestData(pb);

    TEST_ASSERT_NOT_NULL(pb->head);
    for (const PHNode* n = pb->head; n->next != NULL; n = n->next)
    {
        TEST_ASSERT_TRUE_MESSAGE(contact_cmp(&n->data, &n->next->data) <= 0,
                                 "test data must come out sorted");
    }

    phlist_free(&pb);
}

static void test_fillTestData_everyContactIsComplete(void)
{
    PhonebookList* pb = phlist_init();

    fillTestData(pb);

    for (const PHNode* n = pb->head; n != NULL; n = n->next)
    {
        TEST_ASSERT_EQUAL_UINT(MAX_COUNT, (unsigned)n->data.countNumbers);
        TEST_ASSERT_EQUAL_UINT(MAX_COUNT, (unsigned)n->data.countSocial);
        TEST_ASSERT_TRUE(n->data.surname[0] != '\0');
        TEST_ASSERT_TRUE(n->data.name[0] != '\0');
    }

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* addNames                                                            */
/* ------------------------------------------------------------------ */

static void test_addNames_readsSurnameNamePatronymic(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("Petrov\nPetr\nPetrovich\n");

    TEST_ASSERT_EQUAL_INT(1, addNames(&c));
    TEST_ASSERT_EQUAL_STRING("Petrov", c.surname);
    TEST_ASSERT_EQUAL_STRING("Petr", c.name);
    TEST_ASSERT_EQUAL_STRING("Petrovich", c.patronymic);
}

static void test_addNames_emptyPatronymic_becomesDash(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("Petrov\nPetr\n\n");

    TEST_ASSERT_EQUAL_INT(1, addNames(&c));
    TEST_ASSERT_EQUAL_STRING("-", c.patronymic);
}

static void test_addNames_emptySurname_isAskedAgain(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("\nPetrov\nPetr\nPetrovich\n");

    TEST_ASSERT_EQUAL_INT(1, addNames(&c));
    TEST_ASSERT_EQUAL_STRING("Petrov", c.surname);
}

static void test_addNames_eof_returnsZero(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, addNames(&c),
                                  "addNames must report failure on EOF");
}

/* ------------------------------------------------------------------ */
/* addNumber / addSocial                                               */
/* ------------------------------------------------------------------ */

static void test_addNumber_stopsOnEmptyLine(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("+7-900-111\n+7-900-222\n\n");

    addNumber(&c);

    TEST_ASSERT_EQUAL_INT(2, c.countNumbers);
    TEST_ASSERT_EQUAL_STRING("+7-900-111", c.phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("+7-900-222", c.phonenumber[1].number);
}

static void test_addNumber_stopsAtMaxCount(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("1\n2\n3\n4\n5\n");

    addNumber(&c);

    TEST_ASSERT_EQUAL_INT_MESSAGE(MAX_COUNT, c.countNumbers,
                                  "addNumber must not exceed MAX_COUNT");
}

static void test_addNumber_stopsOnEof(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("");

    addNumber(&c);

    TEST_ASSERT_EQUAL_INT(0, c.countNumbers);
}

static void test_addNumber_nullContact_doesNotCrash(void)
{
    addNumber(NULL);
}

static void test_addSocial_readsOneEntry(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("Telegram\nnick\nhttps://t.me/nick\n\n");

    addSocial(&c);

    TEST_ASSERT_EQUAL_INT(1, c.countSocial);
    TEST_ASSERT_EQUAL_STRING("Telegram", c.social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("nick", c.social[0].username);
    TEST_ASSERT_EQUAL_STRING("https://t.me/nick", c.social[0].link);
}

static void test_addSocial_emptyPlatform_stopsImmediately(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    feedStdin("\n");

    addSocial(&c);

    TEST_ASSERT_EQUAL_INT(0, c.countSocial);
}

/* ------------------------------------------------------------------ */
/* addContact                                                          */
/* ------------------------------------------------------------------ */

static void test_addContact_fillsCallersContact(void)
{
    Contact c;
    feedStdin("Petrov\nPetr\nPetrovich\n"
              "+7-900-333\n\n"
              "Telegram\nnick\nhttps://t.me/nick\n\n");

    TEST_ASSERT_EQUAL_INT(1, addContact(&c));
    TEST_ASSERT_EQUAL_STRING("Petrov", c.surname);
    TEST_ASSERT_EQUAL_STRING("Petr", c.name);
    TEST_ASSERT_EQUAL_STRING("Petrovich", c.patronymic);
    TEST_ASSERT_EQUAL_INT(1, c.countNumbers);
    TEST_ASSERT_EQUAL_STRING("+7-900-333", c.phonenumber[0].number);
    TEST_ASSERT_EQUAL_INT(1, c.countSocial);
    TEST_ASSERT_EQUAL_STRING("Telegram", c.social[0].typeSocial);
}

static void test_addContact_eof_reportsFailure(void)
{
    Contact c;
    feedStdin("");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, addContact(&c),
                                  "addContact must report failure on EOF");
}

static void test_addContact_nullArgument_reportsFailure(void)
{
    TEST_ASSERT_EQUAL_INT(0, addContact(NULL));
}

/* ------------------------------------------------------------------ */
/* editNames / editNumber / editSocial                                 */
/* ------------------------------------------------------------------ */

static void test_editNames_replacesAllFields(void)
{
    Contact c = makeContact("Old", "OldName", "OldPatronymic");
    feedStdin("NewName\nNewSurname\nNewPatronymic\n");

    editNames(&c);

    TEST_ASSERT_EQUAL_STRING("NewName", c.name);
    TEST_ASSERT_EQUAL_STRING("NewSurname", c.surname);
    TEST_ASSERT_EQUAL_STRING("NewPatronymic", c.patronymic);
}

static void test_editNames_emptyInput_keepsOldValues(void)
{
    Contact c = makeContact("Old", "OldName", "OldPatronymic");
    feedStdin("\n\n\n");

    editNames(&c);

    TEST_ASSERT_EQUAL_STRING("Old", c.surname);
    TEST_ASSERT_EQUAL_STRING("OldName", c.name);
    TEST_ASSERT_EQUAL_STRING("OldPatronymic", c.patronymic);
}

static void test_editNumber_replacesNumber(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 2, 0);
    feedStdin("+7-900-999\n");

    editNumber(&c, 1);

    TEST_ASSERT_EQUAL_STRING("+7-900-999", c.phonenumber[1].number);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("num0", c.phonenumber[0].number,
                                     "other numbers must stay untouched");
}

static void test_editNumber_indexEqualToCount_rejected(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 2, 0);
    feedStdin("+7-900-999\n");

    editNumber(&c, 2);

    TEST_ASSERT_EQUAL_STRING("num0", c.phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("num1", c.phonenumber[1].number);
}

static void test_editSocial_replacesFields(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 0, 1);
    feedStdin("newuser\nnewtype\nnewlink\n");

    editSocial(&c, 0);

    TEST_ASSERT_EQUAL_STRING("newuser", c.social[0].username);
    TEST_ASSERT_EQUAL_STRING("newtype", c.social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("newlink", c.social[0].link);
}

static void test_editSocial_emptyInput_keepsOldValues(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 0, 1);
    feedStdin("\n\n\n");

    editSocial(&c, 0);

    TEST_ASSERT_EQUAL_STRING("user0", c.social[0].username);
    TEST_ASSERT_EQUAL_STRING("type0", c.social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("link0", c.social[0].link);
}

static void test_editSocial_negativeIndex_changesNothing(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 0, 1);
    feedStdin("newuser\nnewtype\nnewlink\n");

    editSocial(&c, -1);

    TEST_ASSERT_EQUAL_STRING("user0", c.social[0].username);
}

/* ------------------------------------------------------------------ */
/* deleteNumber / deleteSocial                                         */
/* ------------------------------------------------------------------ */

static void test_deleteNumber_middle_shiftsRestLeft(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 3, 0);

    deleteNumber(&c, 1);

    TEST_ASSERT_EQUAL_INT(2, c.countNumbers);
    TEST_ASSERT_EQUAL_STRING("num0", c.phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("num2", c.phonenumber[1].number);
}

static void test_deleteNumber_last_clearsSlot(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 2, 0);

    deleteNumber(&c, 1);

    TEST_ASSERT_EQUAL_INT(1, c.countNumbers);
    TEST_ASSERT_EQUAL_STRING("", c.phonenumber[1].number);
}

static void test_deleteNumber_negativeIndex_changesNothing(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 2, 0);

    deleteNumber(&c, -1);

    TEST_ASSERT_EQUAL_INT(2, c.countNumbers);
    TEST_ASSERT_EQUAL_STRING("num0", c.phonenumber[0].number);
}

/* Граница проверяется как index >= countNumbers - индекс, равный количеству,
   уже за пределами заполненной части массива. */
static void test_deleteNumber_indexEqualToCount_rejected(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 2, 0);

    deleteNumber(&c, 2);

    TEST_ASSERT_EQUAL_INT(2, c.countNumbers);
    TEST_ASSERT_EQUAL_STRING("num0", c.phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("num1", c.phonenumber[1].number);
}

static void test_deleteSocial_middle_shiftsRestLeft(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 0, 3);

    deleteSocial(&c, 1);

    TEST_ASSERT_EQUAL_INT(2, c.countSocial);
    TEST_ASSERT_EQUAL_STRING("type0", c.social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("type2", c.social[1].typeSocial);
}

static void test_deleteSocial_last_clearsSlot(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 0, 2);

    deleteSocial(&c, 1);

    TEST_ASSERT_EQUAL_INT(1, c.countSocial);
    TEST_ASSERT_EQUAL_STRING("", c.social[1].typeSocial);
    TEST_ASSERT_EQUAL_STRING("", c.social[1].username);
    TEST_ASSERT_EQUAL_STRING("", c.social[1].link);
}

static void test_deleteSocial_indexEqualToCount_rejected(void)
{
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    fillFields(&c, 0, 2);

    deleteSocial(&c, 2);

    TEST_ASSERT_EQUAL_INT(2, c.countSocial);
    TEST_ASSERT_EQUAL_STRING("type0", c.social[0].typeSocial);
    TEST_ASSERT_EQUAL_STRING("type1", c.social[1].typeSocial);
}

/* ------------------------------------------------------------------ */
/* Сценарий целиком                                                    */
/* ------------------------------------------------------------------ */

/*
 * Повторяет то, что делает меню: наполнение, удаление по номеру,
 * переименование с перестановкой. Проверяет, что инварианты держатся
 * на всей последовательности операций, а не только по отдельности.
 */
static void test_scenario_insertEraseRepositionKeepOrder(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "Delta");
    push(pb, "Alpha");
    push(pb, "Charlie");
    push(pb, "Bravo");
    assertListValid(pb);
    TEST_ASSERT_EQUAL_STRING("Alpha,Bravo,Charlie,Delta", joinForward(pb));

    phlist_erase(pb, phlist_at(pb, 2));
    assertListValid(pb);
    TEST_ASSERT_EQUAL_STRING("Alpha,Bravo,Delta", joinForward(pb));

    PHNode* node = phlist_at(pb, 0);
    strcpy(node->data.surname, "Echo");
    phlist_reposition(pb, node);
    assertListValid(pb);
    TEST_ASSERT_EQUAL_STRING("Bravo,Delta,Echo", joinForward(pb));
    TEST_ASSERT_EQUAL_STRING("Echo,Delta,Bravo", joinBackward(pb));

    push(pb, "Alpha");
    assertListValid(pb);
    TEST_ASSERT_EQUAL_STRING("Alpha,Bravo,Delta,Echo", joinForward(pb));
    TEST_ASSERT_EQUAL_UINT(4, (unsigned)pb->size);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);

    UNITY_BEGIN();

    RUN_TEST(test_contactCmp_identicalContacts_giveZero);
    RUN_TEST(test_contactCmp_surnameDecidesFirst);
    RUN_TEST(test_contactCmp_nameDecidesWhenSurnamesMatch);
    RUN_TEST(test_contactCmp_patronymicDecidesLast);

    RUN_TEST(test_init_returnsEmptyList);
    RUN_TEST(test_free_nullsCallersPointer);
    RUN_TEST(test_free_onEmptyList_doesNotCrash);
    RUN_TEST(test_free_nullArgument_doesNotCrash);

    RUN_TEST(test_insert_intoEmptyList_becomesHeadAndTail);
    RUN_TEST(test_insert_smallest_goesToHead);
    RUN_TEST(test_insert_largest_goesToTail);
    RUN_TEST(test_insert_middle_linksBothNeighbours);
    RUN_TEST(test_insert_keepsOrderRegardlessOfInsertionOrder);
    RUN_TEST(test_insert_backwardTraversalMatchesForward);
    RUN_TEST(test_insert_ordersByNameWhenSurnamesMatch);
    RUN_TEST(test_insert_equalContacts_keepInsertionOrder);
    RUN_TEST(test_insert_copiesContactByValue);
    RUN_TEST(test_insert_nullArguments_returnNull);
    RUN_TEST(test_insert_incrementsSize);

    RUN_TEST(test_at_returnsNodesInSortedOrder);
    RUN_TEST(test_at_firstAndLastMatchHeadAndTail);
    RUN_TEST(test_at_indexEqualToSize_returnsNull);
    RUN_TEST(test_at_onEmptyList_returnsNull);
    RUN_TEST(test_at_nullList_returnsNull);

    RUN_TEST(test_erase_head_movesHeadForward);
    RUN_TEST(test_erase_tail_movesTailBackward);
    RUN_TEST(test_erase_middle_linksNeighbours);
    RUN_TEST(test_erase_onlyNode_emptiesList);
    RUN_TEST(test_erase_everything_leavesUsableList);
    RUN_TEST(test_erase_decrementsSize);
    RUN_TEST(test_erase_nullArguments_doNotCrash);

    RUN_TEST(test_reposition_renamedToSmallest_movesToHead);
    RUN_TEST(test_reposition_renamedToLargest_movesToTail);
    RUN_TEST(test_reposition_renamedToMiddle_landsBetween);
    RUN_TEST(test_reposition_reusesTheSameNode);
    RUN_TEST(test_reposition_keepsSize);
    RUN_TEST(test_reposition_unchangedKey_keepsPosition);
    RUN_TEST(test_reposition_singleNode_staysHeadAndTail);
    RUN_TEST(test_reposition_nullArguments_doNotCrash);

    RUN_TEST(test_fillTestData_addsAllTestContacts);
    RUN_TEST(test_fillTestData_producesSortedList);
    RUN_TEST(test_fillTestData_everyContactIsComplete);

    RUN_TEST(test_addNames_readsSurnameNamePatronymic);
    RUN_TEST(test_addNames_emptyPatronymic_becomesDash);
    RUN_TEST(test_addNames_emptySurname_isAskedAgain);
    RUN_TEST(test_addNames_eof_returnsZero);

    RUN_TEST(test_addNumber_stopsOnEmptyLine);
    RUN_TEST(test_addNumber_stopsAtMaxCount);
    RUN_TEST(test_addNumber_stopsOnEof);
    RUN_TEST(test_addNumber_nullContact_doesNotCrash);

    RUN_TEST(test_addSocial_readsOneEntry);
    RUN_TEST(test_addSocial_emptyPlatform_stopsImmediately);

    RUN_TEST(test_addContact_fillsCallersContact);
    RUN_TEST(test_addContact_eof_reportsFailure);
    RUN_TEST(test_addContact_nullArgument_reportsFailure);

    RUN_TEST(test_editNames_replacesAllFields);
    RUN_TEST(test_editNames_emptyInput_keepsOldValues);
    RUN_TEST(test_editNumber_replacesNumber);
    RUN_TEST(test_editNumber_indexEqualToCount_rejected);
    RUN_TEST(test_editSocial_replacesFields);
    RUN_TEST(test_editSocial_emptyInput_keepsOldValues);
    RUN_TEST(test_editSocial_negativeIndex_changesNothing);

    RUN_TEST(test_deleteNumber_middle_shiftsRestLeft);
    RUN_TEST(test_deleteNumber_last_clearsSlot);
    RUN_TEST(test_deleteNumber_negativeIndex_changesNothing);
    RUN_TEST(test_deleteNumber_indexEqualToCount_rejected);

    RUN_TEST(test_deleteSocial_middle_shiftsRestLeft);
    RUN_TEST(test_deleteSocial_last_clearsSlot);
    RUN_TEST(test_deleteSocial_indexEqualToCount_rejected);

    RUN_TEST(test_scenario_insertEraseRepositionKeepOrder);

    remove(STDIN_FIXTURE);
    return UNITY_END();
}
