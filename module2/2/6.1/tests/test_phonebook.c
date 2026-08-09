/*
 * Юнит-тесты для libphonebook (задание 6.1)
 *
 * Тест собирается ровно так же, как и сама программа: подключает только
 * заголовок библиотеки и линкуется с libphonebook.a. Поэтому он проверяет
 * не только логику списка, но и то, что библиотека собралась и отдаёт нужные
 * символы - при неверной сборке тест просто не слинкуется.
 *
 * Меню, ввод и редактирование контактов в библиотеку не входят: они лежат
 * в menu.c и с задания 4.1 не менялись, там же они и покрыты тестами.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h> /* SetConsoleOutputCP, CP_UTF8 */
#endif

#include "unity/unity.h"
#include "../phoneBook.h"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

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

/* Инварианты цепочки, которые обязана сохранять любая операция. */
static void assertListIsConsistent(const PhonebookList* pb)
{
    if (pb->head == NULL)
    {
        TEST_ASSERT_NULL_MESSAGE(pb->tail, "tail must be NULL in an empty list");
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, (unsigned)pb->size,
                                       "size must be 0 in an empty list");
        return;
    }

    TEST_ASSERT_NULL_MESSAGE(pb->head->prev, "head->prev must be NULL");
    TEST_ASSERT_NULL_MESSAGE(pb->tail->next, "tail->next must be NULL");

    size_t counted = 0;
    for (const PHNode* n = pb->head; n != NULL; n = n->next)
    {
        if (n->next != NULL)
        {
            TEST_ASSERT_EQUAL_PTR_MESSAGE(n, n->next->prev,
                                          "next->prev must point back");
        }
        counted++;
    }

    TEST_ASSERT_EQUAL_UINT_MESSAGE((unsigned)pb->size, (unsigned)counted,
                                   "size must match the number of nodes");
}

void setUp(void)
{
}

void tearDown(void)
{
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
    Contact a = makeContact("Aaa", "Zzz", "Zzz");
    Contact b = makeContact("Bbb", "Aaa", "Aaa");

    TEST_ASSERT_TRUE(contact_cmp(&a, &b) < 0);
    TEST_ASSERT_TRUE(contact_cmp(&b, &a) > 0);
}

static void test_contactCmp_nameThenPatronymic(void)
{
    Contact a = makeContact("Ivanov", "Aaa", "Zzz");
    Contact b = makeContact("Ivanov", "Bbb", "Aaa");
    Contact c = makeContact("Ivanov", "Bbb", "Bbb");

    TEST_ASSERT_TRUE(contact_cmp(&a, &b) < 0);
    TEST_ASSERT_TRUE(contact_cmp(&b, &c) < 0);
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

    phlist_free(&pb);
}

static void test_free_nullsCallersPointer(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "Ivanov");

    phlist_free(&pb);

    TEST_ASSERT_NULL(pb);
}

static void test_free_nullArguments_doNotCrash(void)
{
    PhonebookList* pb = NULL;

    phlist_free(&pb);
    phlist_free(NULL);

    TEST_PASS();
}

/* ------------------------------------------------------------------ */
/* phlist_insert                                                       */
/* ------------------------------------------------------------------ */

static void test_insert_intoEmptyList_becomesHeadAndTail(void)
{
    PhonebookList* pb = phlist_init();

    PHNode* node = push(pb, "Ivanov");

    TEST_ASSERT_EQUAL_PTR(node, pb->head);
    TEST_ASSERT_EQUAL_PTR(node, pb->tail);
    TEST_ASSERT_NULL(node->prev);
    TEST_ASSERT_NULL(node->next);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);

    phlist_free(&pb);
}

static void test_insert_keepsListSortedRegardlessOfOrder(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "D");
    push(pb, "A");
    push(pb, "F");
    push(pb, "C");

    TEST_ASSERT_EQUAL_STRING("A,C,D,F", joinForward(pb));
    assertListIsConsistent(pb);

    phlist_free(&pb);
}

static void test_insert_backwardTraversalMatchesForward(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "B");
    push(pb, "A");
    push(pb, "C");

    TEST_ASSERT_EQUAL_STRING("A,B,C", joinForward(pb));
    TEST_ASSERT_EQUAL_STRING("C,B,A", joinBackward(pb));

    phlist_free(&pb);
}

static void test_insert_smallest_goesToHead(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "B");
    PHNode* node = push(pb, "A");

    TEST_ASSERT_EQUAL_PTR(node, pb->head);
    TEST_ASSERT_NULL(node->prev);

    phlist_free(&pb);
}

static void test_insert_largest_goesToTail(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "B");
    PHNode* node = push(pb, "C");

    TEST_ASSERT_EQUAL_PTR(node, pb->tail);
    TEST_ASSERT_NULL(node->next);

    phlist_free(&pb);
}

/* Контакты с одинаковым Ф.И.О. встают в конец своей группы -
   порядок добавления сохраняется. */
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

    phlist_free(&pb);
}

static void test_insert_copiesContactByValue(void)
{
    PhonebookList* pb = phlist_init();

    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    PHNode* node = phlist_insert(pb, &c);

    strcpy(c.surname, "Petrov");

    TEST_ASSERT_EQUAL_STRING("Ivanov", node->data.surname);

    phlist_free(&pb);
}

static void test_insert_nullArguments_returnNull(void)
{
    PhonebookList* pb = phlist_init();
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");

    TEST_ASSERT_NULL(phlist_insert(NULL, &c));
    TEST_ASSERT_NULL(phlist_insert(pb, NULL));
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->size);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phlist_at                                                           */
/* ------------------------------------------------------------------ */

static void test_at_returnsNodesInSortedOrder(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "C");
    push(pb, "A");
    push(pb, "B");

    TEST_ASSERT_EQUAL_STRING("A", phlist_at(pb, 0)->data.surname);
    TEST_ASSERT_EQUAL_STRING("B", phlist_at(pb, 1)->data.surname);
    TEST_ASSERT_EQUAL_STRING("C", phlist_at(pb, 2)->data.surname);

    phlist_free(&pb);
}

static void test_at_firstAndLastMatchHeadAndTail(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "A");
    push(pb, "B");

    TEST_ASSERT_EQUAL_PTR(pb->head, phlist_at(pb, 0));
    TEST_ASSERT_EQUAL_PTR(pb->tail, phlist_at(pb, 1));

    phlist_free(&pb);
}

static void test_at_outOfRange_returnsNull(void)
{
    PhonebookList* pb = phlist_init();
    push(pb, "A");

    TEST_ASSERT_NULL(phlist_at(pb, 1));
    TEST_ASSERT_NULL(phlist_at(pb, 100));
    TEST_ASSERT_NULL(phlist_at(NULL, 0));

    phlist_free(&pb);
}

static void test_at_onEmptyList_returnsNull(void)
{
    PhonebookList* pb = phlist_init();

    TEST_ASSERT_NULL(phlist_at(pb, 0));

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phlist_erase                                                        */
/* ------------------------------------------------------------------ */

static void test_erase_head_movesHeadForward(void)
{
    PhonebookList* pb = phlist_init();

    PHNode* first = push(pb, "A");
    push(pb, "B");

    phlist_erase(pb, first);

    TEST_ASSERT_EQUAL_STRING("B", pb->head->data.surname);
    TEST_ASSERT_NULL(pb->head->prev);
    assertListIsConsistent(pb);

    phlist_free(&pb);
}

static void test_erase_tail_movesTailBackward(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "A");
    PHNode* last = push(pb, "B");

    phlist_erase(pb, last);

    TEST_ASSERT_EQUAL_STRING("A", pb->tail->data.surname);
    TEST_ASSERT_NULL(pb->tail->next);
    assertListIsConsistent(pb);

    phlist_free(&pb);
}

static void test_erase_middle_linksNeighbours(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "A");
    PHNode* middle = push(pb, "B");
    push(pb, "C");

    phlist_erase(pb, middle);

    TEST_ASSERT_EQUAL_STRING("A,C", joinForward(pb));
    TEST_ASSERT_EQUAL_STRING("C,A", joinBackward(pb));
    assertListIsConsistent(pb);

    phlist_free(&pb);
}

static void test_erase_onlyNode_emptiesList(void)
{
    PhonebookList* pb = phlist_init();
    PHNode* node = push(pb, "A");

    phlist_erase(pb, node);

    TEST_ASSERT_NULL(pb->head);
    TEST_ASSERT_NULL(pb->tail);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->size);

    phlist_free(&pb);
}

static void test_erase_everything_leavesUsableList(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "A");
    push(pb, "B");
    push(pb, "C");

    while (pb->size > 0)
    {
        phlist_erase(pb, pb->head);
    }

    push(pb, "New");

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_STRING("New", pb->head->data.surname);
    assertListIsConsistent(pb);

    phlist_free(&pb);
}

static void test_erase_nullArguments_doNotCrash(void)
{
    PhonebookList* pb = phlist_init();
    PHNode* node = push(pb, "A");

    phlist_erase(NULL, node);
    phlist_erase(pb, NULL);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phlist_reposition                                                   */
/* ------------------------------------------------------------------ */

static void test_reposition_renamedToLargest_movesToTail(void)
{
    PhonebookList* pb = phlist_init();

    PHNode* node = push(pb, "A");
    push(pb, "B");
    push(pb, "C");

    strcpy(node->data.surname, "Z");
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_STRING("B,C,Z", joinForward(pb));
    TEST_ASSERT_EQUAL_PTR(node, pb->tail);
    assertListIsConsistent(pb);

    phlist_free(&pb);
}

static void test_reposition_renamedToSmallest_movesToHead(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "A");
    push(pb, "B");
    PHNode* node = push(pb, "C");

    strcpy(node->data.surname, "0");
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_PTR(node, pb->head);
    TEST_ASSERT_EQUAL_STRING("0,A,B", joinForward(pb));

    phlist_free(&pb);
}

/* Перестановка переиспользует тот же узел, память не перевыделяется. */
static void test_reposition_reusesTheSameNode_andKeepsSize(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "A");
    PHNode* node = push(pb, "B");
    push(pb, "C");

    strcpy(node->data.surname, "Z");
    phlist_reposition(pb, node);

    TEST_ASSERT_EQUAL_PTR(node, phlist_at(pb, 2));
    TEST_ASSERT_EQUAL_UINT(3, (unsigned)pb->size);

    phlist_free(&pb);
}

static void test_reposition_nullArguments_doNotCrash(void)
{
    PhonebookList* pb = phlist_init();
    PHNode* node = push(pb, "A");

    phlist_reposition(NULL, node);
    phlist_reposition(pb, NULL);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);
    assertListIsConsistent(pb);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */
/* Сценарий целиком                                                    */
/* ------------------------------------------------------------------ */

static void test_scenario_insertEraseRepositionKeepOrder(void)
{
    PhonebookList* pb = phlist_init();

    push(pb, "D");
    push(pb, "B");
    PHNode* f = push(pb, "F");
    push(pb, "A");
    PHNode* c = push(pb, "C");

    TEST_ASSERT_EQUAL_STRING("A,B,C,D,F", joinForward(pb));

    phlist_erase(pb, f);
    TEST_ASSERT_EQUAL_STRING("A,B,C,D", joinForward(pb));

    strcpy(c->data.surname, "Z");
    phlist_reposition(pb, c);
    TEST_ASSERT_EQUAL_STRING("A,B,D,Z", joinForward(pb));

    push(pb, "E");
    TEST_ASSERT_EQUAL_STRING("A,B,D,E,Z", joinForward(pb));
    TEST_ASSERT_EQUAL_STRING("Z,E,D,B,A", joinBackward(pb));

    assertListIsConsistent(pb);
    TEST_ASSERT_EQUAL_UINT(5, (unsigned)pb->size);

    phlist_free(&pb);
}

/* ------------------------------------------------------------------ */

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    UNITY_BEGIN();

    RUN_TEST(test_contactCmp_identicalContacts_giveZero);
    RUN_TEST(test_contactCmp_surnameDecidesFirst);
    RUN_TEST(test_contactCmp_nameThenPatronymic);

    RUN_TEST(test_init_returnsEmptyList);
    RUN_TEST(test_free_nullsCallersPointer);
    RUN_TEST(test_free_nullArguments_doNotCrash);

    RUN_TEST(test_insert_intoEmptyList_becomesHeadAndTail);
    RUN_TEST(test_insert_keepsListSortedRegardlessOfOrder);
    RUN_TEST(test_insert_backwardTraversalMatchesForward);
    RUN_TEST(test_insert_smallest_goesToHead);
    RUN_TEST(test_insert_largest_goesToTail);
    RUN_TEST(test_insert_equalContacts_keepInsertionOrder);
    RUN_TEST(test_insert_copiesContactByValue);
    RUN_TEST(test_insert_nullArguments_returnNull);

    RUN_TEST(test_at_returnsNodesInSortedOrder);
    RUN_TEST(test_at_firstAndLastMatchHeadAndTail);
    RUN_TEST(test_at_outOfRange_returnsNull);
    RUN_TEST(test_at_onEmptyList_returnsNull);

    RUN_TEST(test_erase_head_movesHeadForward);
    RUN_TEST(test_erase_tail_movesTailBackward);
    RUN_TEST(test_erase_middle_linksNeighbours);
    RUN_TEST(test_erase_onlyNode_emptiesList);
    RUN_TEST(test_erase_everything_leavesUsableList);
    RUN_TEST(test_erase_nullArguments_doNotCrash);

    RUN_TEST(test_reposition_renamedToLargest_movesToTail);
    RUN_TEST(test_reposition_renamedToSmallest_movesToHead);
    RUN_TEST(test_reposition_reusesTheSameNode_andKeepsSize);
    RUN_TEST(test_reposition_nullArguments_doNotCrash);

    RUN_TEST(test_scenario_insertEraseRepositionKeepOrder);

    return UNITY_END();
}
