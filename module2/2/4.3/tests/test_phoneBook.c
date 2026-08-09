/*
 * Юнит-тесты для phoneBook.c (задание 4.3)
 *
 * От 4.1 задание отличается только хранением - двоичное дерево поиска вместо
 * списка, поэтому здесь проверяется именно оно: инвариант порядка, связи с
 * родителем, три случая удаления и периодическая балансировка. Ввод контакта,
 * редактирование и удаление номеров/соцсетей не тронуты по сравнению с 4.1
 * и покрыты тестами там.
 *
 * Часть функций в phoneBook.c объявлена static, слинковать их из отдельного
 * объектного файла нельзя. Стандартный приём - включить сам .c-файл в тест:
 * тест и тестируемый код попадают в одну единицу трансляции, и static
 * перестаёт мешать. main() берётся из этого файла, main.c в сборку не входит.
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
#include "../phoneBook.c"

#define STDIN_FIXTURE  "stdin_fixture.tmp"
#define STDOUT_CAPTURE "stdout_capture.tmp"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

/* Подменяет stdin содержимым строки - так тестируется меню. */
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
 * Меню и сообщение о перестроении дерева печатаются в stdout. Перехватываем
 * его в файл: часть тестов проверяет напечатанное, остальным важно лишь, чтобы
 * отчёт Unity не тонул в выводе программы.
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

static const char* endCaptureStdout(void)
{
    static char buffer[16384];

    restoreStdout();

    FILE* f = fopen(STDOUT_CAPTURE, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "не удалось открыть файл перехвата stdout");

    size_t readBytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[readBytes] = '\0';
    fclose(f);

    return buffer;
}

static const char* runInterface(const char* input)
{
    feedStdin(input);
    captureStdout();
    print_interface();
    return endCaptureStdout();
}

/* Unity экранирует любой байт вне ASCII, поэтому сообщения - латиницей. */
static void assertContains(const char* haystack, const char* needle,
                           const char* message)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(haystack, needle), message);
}

/* Фамилии тоже латиницей - иначе не прочитать сообщение об упавшем сравнении. */
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

static PHNode* push(PhonebookTree* pb, const char* surname)
{
    Contact c = makeContact(surname, "Name", "Patronymic");
    return phtree_insert(pb, &c);
}

/* Фамилии в порядке обхода дерева слева направо. */
static const char* joinInorder(const PhonebookTree* pb)
{
    static char buffer[1024];
    buffer[0] = '\0';

    for (size_t i = 0; i < pb->size; ++i)
    {
        const PHNode* node = phtree_at(pb, i);
        TEST_ASSERT_NOT_NULL_MESSAGE(node, "phtree_at returned NULL inside size");

        if (buffer[0] != '\0') strcat(buffer, ",");
        strcat(buffer, node->data.surname);
    }

    return buffer;
}

/* Указатель на родителя обязан быть согласован со ссылкой сверху вниз. */
static void assertParents(const PHNode* node, const PHNode* parent)
{
    if (node == NULL) return;

    TEST_ASSERT_EQUAL_PTR_MESSAGE(parent, node->parent, "parent link is broken");

    assertParents(node->left, node);
    assertParents(node->right, node);
}

/* Обход слева направо обязан давать неубывающую последовательность. */
static void assertSorted(const PhonebookTree* pb)
{
    for (size_t i = 1; i < pb->size; ++i)
    {
        const PHNode* prev = phtree_at(pb, i - 1);
        const PHNode* curr = phtree_at(pb, i);

        TEST_ASSERT_TRUE_MESSAGE(contact_cmp(&prev->data, &curr->data) <= 0,
                                 "tree order is broken");
    }
}

/* Дерево из фамилий "S00", "S01", ... - строго по возрастанию,
   то есть вырожденное в цепочку вправо. */
static PhonebookTree* makeDegenerate(size_t count)
{
    PhonebookTree* pb = phtree_init();
    TEST_ASSERT_NOT_NULL(pb);

    for (size_t i = 0; i < count; ++i)
    {
        char surname[MAX_SIZE];
        snprintf(surname, sizeof(surname), "S%02u", (unsigned)i);
        push(pb, surname);
    }

    return pb;
}

void setUp(void)
{
}

void tearDown(void)
{
    restoreStdout();
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
/* phtree_init / phtree_free                                           */
/* ------------------------------------------------------------------ */

static void test_init_returnsEmptyTree(void)
{
    PhonebookTree* pb = phtree_init();

    TEST_ASSERT_NOT_NULL(pb);
    TEST_ASSERT_NULL(pb->root);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->opsSinceBalance);

    phtree_free(&pb);
}

static void test_free_nullsCallersPointer(void)
{
    PhonebookTree* pb = phtree_init();
    push(pb, "Ivanov");

    phtree_free(&pb);

    TEST_ASSERT_NULL(pb);
}

static void test_free_nullArguments_doNotCrash(void)
{
    PhonebookTree* pb = NULL;

    phtree_free(&pb);
    phtree_free(NULL);

    TEST_PASS();
}

/* ------------------------------------------------------------------ */
/* phtree_insert                                                       */
/* ------------------------------------------------------------------ */

static void test_insert_intoEmptyTree_becomesRoot(void)
{
    PhonebookTree* pb = phtree_init();

    PHNode* node = push(pb, "Ivanov");

    TEST_ASSERT_EQUAL_PTR(node, pb->root);
    TEST_ASSERT_NULL(node->parent);
    TEST_ASSERT_NULL(node->left);
    TEST_ASSERT_NULL(node->right);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);

    phtree_free(&pb);
}

static void test_insert_smallerGoesLeft_biggerGoesRight(void)
{
    PhonebookTree* pb = phtree_init();

    PHNode* root = push(pb, "M");
    PHNode* left = push(pb, "A");
    PHNode* right = push(pb, "Z");

    TEST_ASSERT_EQUAL_PTR(left, root->left);
    TEST_ASSERT_EQUAL_PTR(right, root->right);
    TEST_ASSERT_EQUAL_PTR(root, left->parent);
    TEST_ASSERT_EQUAL_PTR(root, right->parent);

    phtree_free(&pb);
}

static void test_insert_orderDoesNotDependOnInsertionOrder(void)
{
    PhonebookTree* first = phtree_init();
    PhonebookTree* second = phtree_init();

    const char* forward[] = { "A", "B", "C", "D", "E", "F", "G" };
    const char* shuffled[] = { "D", "B", "F", "A", "C", "E", "G" };

    for (int i = 0; i < 7; ++i)
    {
        push(first, forward[i]);
        push(second, shuffled[i]);
    }

    TEST_ASSERT_EQUAL_STRING("A,B,C,D,E,F,G", joinInorder(first));
    TEST_ASSERT_EQUAL_STRING("A,B,C,D,E,F,G", joinInorder(second));

    phtree_free(&first);
    phtree_free(&second);
}

static void test_insert_keepsParentLinksConsistent(void)
{
    PhonebookTree* pb = phtree_init();

    const char* surnames[] = { "D", "B", "F", "A", "C", "E", "G" };
    for (int i = 0; i < 7; ++i) push(pb, surnames[i]);

    assertParents(pb->root, NULL);

    phtree_free(&pb);
}

/* Однофамильцы с совпадающим Ф.И.О. уходят вправо, то есть добавляются
   в конец своей группы - порядок добавления сохраняется. */
static void test_insert_equalContacts_keepInsertionOrder(void)
{
    PhonebookTree* pb = phtree_init();

    Contact first = makeContact("Ivanov", "Ivan", "Ivanovich");
    strcpy(first.phonenumber[0].number, "first");
    first.countNumbers = 1;

    Contact second = makeContact("Ivanov", "Ivan", "Ivanovich");
    strcpy(second.phonenumber[0].number, "second");
    second.countNumbers = 1;

    phtree_insert(pb, &first);
    phtree_insert(pb, &second);

    TEST_ASSERT_EQUAL_UINT(2, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_STRING("first", phtree_at(pb, 0)->data.phonenumber[0].number);
    TEST_ASSERT_EQUAL_STRING("second", phtree_at(pb, 1)->data.phonenumber[0].number);

    phtree_free(&pb);
}

static void test_insert_copiesContactByValue(void)
{
    PhonebookTree* pb = phtree_init();

    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");
    PHNode* node = phtree_insert(pb, &c);

    strcpy(c.surname, "Petrov");

    TEST_ASSERT_EQUAL_STRING("Ivanov", node->data.surname);

    phtree_free(&pb);
}

static void test_insert_nullArguments_returnNull(void)
{
    PhonebookTree* pb = phtree_init();
    Contact c = makeContact("Ivanov", "Ivan", "Ivanovich");

    TEST_ASSERT_NULL(phtree_insert(NULL, &c));
    TEST_ASSERT_NULL(phtree_insert(pb, NULL));
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->size);

    phtree_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phtree_at                                                           */
/* ------------------------------------------------------------------ */

static void test_at_returnsNodesInSortedOrder(void)
{
    PhonebookTree* pb = phtree_init();

    push(pb, "D");
    push(pb, "B");
    push(pb, "F");

    TEST_ASSERT_EQUAL_STRING("B", phtree_at(pb, 0)->data.surname);
    TEST_ASSERT_EQUAL_STRING("D", phtree_at(pb, 1)->data.surname);
    TEST_ASSERT_EQUAL_STRING("F", phtree_at(pb, 2)->data.surname);

    phtree_free(&pb);
}

static void test_at_outOfRange_returnsNull(void)
{
    PhonebookTree* pb = phtree_init();
    push(pb, "D");

    TEST_ASSERT_NULL(phtree_at(pb, 1));
    TEST_ASSERT_NULL(phtree_at(pb, 100));
    TEST_ASSERT_NULL(phtree_at(NULL, 0));

    phtree_free(&pb);
}

static void test_at_onEmptyTree_returnsNull(void)
{
    PhonebookTree* pb = phtree_init();

    TEST_ASSERT_NULL(phtree_at(pb, 0));

    phtree_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phtree_erase                                                        */
/* ------------------------------------------------------------------ */

static void test_erase_leaf_detachesFromParent(void)
{
    PhonebookTree* pb = phtree_init();

    PHNode* root = push(pb, "D");
    PHNode* leaf = push(pb, "B");
    push(pb, "F");

    phtree_erase(pb, leaf);

    TEST_ASSERT_NULL(root->left);
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_STRING("D,F", joinInorder(pb));
    assertParents(pb->root, NULL);

    phtree_free(&pb);
}

static void test_erase_nodeWithOneChild_childTakesItsPlace(void)
{
    PhonebookTree* pb = phtree_init();

    PHNode* root = push(pb, "D");
    PHNode* middle = push(pb, "B");
    PHNode* child = push(pb, "A");

    phtree_erase(pb, middle);

    TEST_ASSERT_EQUAL_PTR(child, root->left);
    TEST_ASSERT_EQUAL_PTR(root, child->parent);
    TEST_ASSERT_EQUAL_STRING("A,D", joinInorder(pb));
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)pb->size);

    phtree_free(&pb);
}

/* Узел с двумя детьми: на его место переезжают данные преемника,
   а физически удаляется преемник. Порядок обхода обязан уцелеть. */
static void test_erase_nodeWithTwoChildren_keepsOrder(void)
{
    PhonebookTree* pb = phtree_init();

    push(pb, "D");
    PHNode* target = push(pb, "B");
    push(pb, "F");
    push(pb, "A");
    push(pb, "C");

    phtree_erase(pb, target);

    TEST_ASSERT_EQUAL_UINT(4, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_STRING("A,C,D,F", joinInorder(pb));
    assertParents(pb->root, NULL);
    assertSorted(pb);

    phtree_free(&pb);
}

static void test_erase_rootWithTwoChildren_keepsTreeUsable(void)
{
    PhonebookTree* pb = phtree_init();

    PHNode* root = push(pb, "D");
    push(pb, "B");
    push(pb, "F");

    phtree_erase(pb, root);

    TEST_ASSERT_NOT_NULL(pb->root);
    TEST_ASSERT_NULL(pb->root->parent);
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_STRING("B,F", joinInorder(pb));

    phtree_free(&pb);
}

static void test_erase_onlyNode_emptiesTree(void)
{
    PhonebookTree* pb = phtree_init();
    PHNode* node = push(pb, "D");

    phtree_erase(pb, node);

    TEST_ASSERT_NULL(pb->root);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->size);

    phtree_free(&pb);
}

static void test_erase_everything_leavesUsableTree(void)
{
    PhonebookTree* pb = phtree_init();

    push(pb, "D");
    push(pb, "B");
    push(pb, "F");

    while (pb->size > 0)
    {
        phtree_erase(pb, phtree_at(pb, 0));
    }

    TEST_ASSERT_NULL(pb->root);

    push(pb, "New");
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_STRING("New", pb->root->data.surname);

    phtree_free(&pb);
}

static void test_erase_nullArguments_doNotCrash(void)
{
    PhonebookTree* pb = phtree_init();
    PHNode* node = push(pb, "D");

    phtree_erase(NULL, node);
    phtree_erase(pb, NULL);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);

    phtree_free(&pb);
}

/* ------------------------------------------------------------------ */
/* phtree_reposition                                                   */
/* ------------------------------------------------------------------ */

static void test_reposition_renamedContact_movesToNewPlace(void)
{
    PhonebookTree* pb = phtree_init();

    push(pb, "A");
    push(pb, "B");
    push(pb, "C");

    PHNode* node = phtree_at(pb, 0);
    strcpy(node->data.surname, "Z");

    PHNode* moved = phtree_reposition(pb, node);

    TEST_ASSERT_NOT_NULL(moved);
    TEST_ASSERT_EQUAL_STRING("B,C,Z", joinInorder(pb));
    TEST_ASSERT_EQUAL_UINT(3, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_PTR(moved, phtree_at(pb, 2));

    phtree_free(&pb);
}

/* Данные обязаны пережить перестановку целиком, а не только Ф.И.О. */
static void test_reposition_keepsContactData(void)
{
    PhonebookTree* pb = phtree_init();

    Contact c = makeContact("A", "Name", "Patronymic");
    strcpy(c.phonenumber[0].number, "+7-900-111");
    c.countNumbers = 1;

    PHNode* node = phtree_insert(pb, &c);
    push(pb, "B");

    strcpy(node->data.surname, "Z");
    PHNode* moved = phtree_reposition(pb, node);

    TEST_ASSERT_EQUAL_STRING("+7-900-111", moved->data.phonenumber[0].number);
    TEST_ASSERT_EQUAL_INT(1, moved->data.countNumbers);

    phtree_free(&pb);
}

static void test_reposition_unchangedKey_keepsOrder(void)
{
    PhonebookTree* pb = phtree_init();

    push(pb, "A");
    PHNode* node = push(pb, "B");
    push(pb, "C");

    phtree_reposition(pb, node);

    TEST_ASSERT_EQUAL_STRING("A,B,C", joinInorder(pb));
    assertParents(pb->root, NULL);

    phtree_free(&pb);
}

static void test_reposition_nullArguments_returnNull(void)
{
    PhonebookTree* pb = phtree_init();
    PHNode* node = push(pb, "A");

    TEST_ASSERT_NULL(phtree_reposition(NULL, node));
    TEST_ASSERT_NULL(phtree_reposition(pb, NULL));
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)pb->size);

    phtree_free(&pb);
}

/* ------------------------------------------------------------------ */
/* Высота                                                              */
/* ------------------------------------------------------------------ */

static void test_height_emptyAndSingle(void)
{
    PhonebookTree* pb = phtree_init();

    TEST_ASSERT_EQUAL_UINT(0, (unsigned)phtree_height(pb));

    push(pb, "A");
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)phtree_height(pb));

    phtree_free(&pb);
}

/* Вставка по возрастанию вырождает дерево в список */
static void test_height_ascendingInsertion_degenerates(void)
{
    PhonebookTree* pb = makeDegenerate(5);

    TEST_ASSERT_EQUAL_UINT(5, (unsigned)phtree_height(pb));
    TEST_ASSERT_EQUAL_UINT(5, (unsigned)pb->size);

    phtree_free(&pb);
}

static void test_idealHeight_isLogOfSize(void)
{
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)phtree_ideal_height(0));
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)phtree_ideal_height(1));
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)phtree_ideal_height(2));
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)phtree_ideal_height(3));
    TEST_ASSERT_EQUAL_UINT(3, (unsigned)phtree_ideal_height(4));
    TEST_ASSERT_EQUAL_UINT(3, (unsigned)phtree_ideal_height(7));
    TEST_ASSERT_EQUAL_UINT(4, (unsigned)phtree_ideal_height(8));
    TEST_ASSERT_EQUAL_UINT(5, (unsigned)phtree_ideal_height(16));
}

/* ------------------------------------------------------------------ */
/* phtree_balance                                                      */
/* ------------------------------------------------------------------ */

static void test_balance_degenerateTree_becomesShallow(void)
{
    PhonebookTree* pb = makeDegenerate(7);
    TEST_ASSERT_EQUAL_UINT(7, (unsigned)phtree_height(pb));

    TEST_ASSERT_EQUAL_INT(1, phtree_balance(pb));

    TEST_ASSERT_EQUAL_UINT(3, (unsigned)phtree_height(pb));
    TEST_ASSERT_EQUAL_UINT(7, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_STRING("S00,S01,S02,S03,S04,S05,S06", joinInorder(pb));
    assertParents(pb->root, NULL);
    assertSorted(pb);

    phtree_free(&pb);
}

/* Перестроение переписывает только связи - узлы остаются теми же,
   поэтому указатели, взятые до балансировки, продолжают работать. */
static void test_balance_reusesTheSameNodes(void)
{
    PhonebookTree* pb = makeDegenerate(7);

    PHNode* third = phtree_at(pb, 3);

    phtree_balance(pb);

    TEST_ASSERT_EQUAL_PTR(third, phtree_at(pb, 3));
    TEST_ASSERT_EQUAL_STRING("S03", third->data.surname);

    phtree_free(&pb);
}

static void test_balance_resetsCounter(void)
{
    PhonebookTree* pb = makeDegenerate(5);
    TEST_ASSERT_EQUAL_UINT(5, (unsigned)pb->opsSinceBalance);

    phtree_balance(pb);

    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->opsSinceBalance);

    phtree_free(&pb);
}

static void test_balance_tooSmallTree_reportsNothingToDo(void)
{
    PhonebookTree* pb = makeDegenerate(2);

    TEST_ASSERT_EQUAL_INT(0, phtree_balance(pb));
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)pb->size);

    phtree_free(&pb);
}

static void test_balance_nullArgument_doesNotCrash(void)
{
    TEST_ASSERT_EQUAL_INT(0, phtree_balance(NULL));
}

/* ------------------------------------------------------------------ */
/* Периодическая балансировка                                          */
/* ------------------------------------------------------------------ */

/*
 * Проверка запускается раз в BALANCE_PERIOD изменений, но перестраивает
 * дерево только при высоте больше BALANCE_FACTOR идеальных. На восьми узлах
 * порог как раз равен высоте вырожденной цепочки, поэтому перестроения ещё
 * не происходит - счётчик просто сбрасывается.
 */
static void test_autoBalance_withinThreshold_doesNotRebuild(void)
{
    captureStdout();
    PhonebookTree* pb = makeDegenerate(BALANCE_PERIOD);
    restoreStdout();

    TEST_ASSERT_EQUAL_UINT(BALANCE_PERIOD, (unsigned)phtree_height(pb));
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)pb->opsSinceBalance);

    phtree_free(&pb);
}

static void test_autoBalance_degenerateTree_rebuildsItself(void)
{
    captureStdout();
    PhonebookTree* pb = makeDegenerate(2 * BALANCE_PERIOD);
    const char* out = endCaptureStdout();

    TEST_ASSERT_EQUAL_UINT(2 * BALANCE_PERIOD, (unsigned)pb->size);
    TEST_ASSERT_EQUAL_UINT(5, (unsigned)phtree_height(pb));
    assertParents(pb->root, NULL);
    assertSorted(pb);
    assertContains(out, "перестроено", "rebuild is expected to be reported");

    phtree_free(&pb);
}

static void test_autoBalance_keepsAllContacts(void)
{
    captureStdout();
    PhonebookTree* pb = makeDegenerate(2 * BALANCE_PERIOD);
    restoreStdout();

    TEST_ASSERT_EQUAL_STRING("S00", phtree_at(pb, 0)->data.surname);
    TEST_ASSERT_EQUAL_STRING("S15", phtree_at(pb, 15)->data.surname);

    phtree_free(&pb);
}

/* ------------------------------------------------------------------ */
/* fillTestData                                                        */
/* ------------------------------------------------------------------ */

static void test_fillTestData_addsAllTestContacts(void)
{
    PhonebookTree* pb = phtree_init();

    captureStdout();
    fillTestData(pb);
    restoreStdout();

    TEST_ASSERT_EQUAL_UINT(MAX_COUNT_CONTACTS, (unsigned)pb->size);
    assertSorted(pb);
    assertParents(pb->root, NULL);

    phtree_free(&pb);
}

static void test_fillTestData_everyContactIsComplete(void)
{
    PhonebookTree* pb = phtree_init();

    captureStdout();
    fillTestData(pb);
    restoreStdout();

    for (size_t i = 0; i < pb->size; ++i)
    {
        const Contact* c = &phtree_at(pb, i)->data;

        TEST_ASSERT_EQUAL_INT(MAX_COUNT, c->countNumbers);
        TEST_ASSERT_EQUAL_INT(MAX_COUNT, c->countSocial);
        TEST_ASSERT_TRUE(c->surname[0] != '\0');
    }

    phtree_free(&pb);
}

/* ------------------------------------------------------------------ */
/* Меню                                                                */
/* ------------------------------------------------------------------ */

static void test_interface_zero_exitsImmediately(void)
{
    const char* out = runInterface("0\n");

    assertContains(out, "Выберите действие", "menu prompt is expected");
}

static void test_interface_treeItem_showsHeightAndShape(void)
{
    const char* out = runInterface("5\n0\n");

    assertContains(out, "Высота дерева", "tree height is expected");
    assertContains(out, "Контактов", "tree size is expected");
}

static void test_interface_balanceItem_reportsNewHeight(void)
{
    const char* out = runInterface("6\n0\n");

    assertContains(out, "Высота дерева", "height before and after is expected");
}

static void test_interface_printItem_listsTestContacts(void)
{
    const char* out = runInterface("4\n0\n");

    assertContains(out, "Контакт #1", "first contact is expected");
    assertContains(out, "Контакт #8", "all eight test contacts are expected");
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

    RUN_TEST(test_init_returnsEmptyTree);
    RUN_TEST(test_free_nullsCallersPointer);
    RUN_TEST(test_free_nullArguments_doNotCrash);

    RUN_TEST(test_insert_intoEmptyTree_becomesRoot);
    RUN_TEST(test_insert_smallerGoesLeft_biggerGoesRight);
    RUN_TEST(test_insert_orderDoesNotDependOnInsertionOrder);
    RUN_TEST(test_insert_keepsParentLinksConsistent);
    RUN_TEST(test_insert_equalContacts_keepInsertionOrder);
    RUN_TEST(test_insert_copiesContactByValue);
    RUN_TEST(test_insert_nullArguments_returnNull);

    RUN_TEST(test_at_returnsNodesInSortedOrder);
    RUN_TEST(test_at_outOfRange_returnsNull);
    RUN_TEST(test_at_onEmptyTree_returnsNull);

    RUN_TEST(test_erase_leaf_detachesFromParent);
    RUN_TEST(test_erase_nodeWithOneChild_childTakesItsPlace);
    RUN_TEST(test_erase_nodeWithTwoChildren_keepsOrder);
    RUN_TEST(test_erase_rootWithTwoChildren_keepsTreeUsable);
    RUN_TEST(test_erase_onlyNode_emptiesTree);
    RUN_TEST(test_erase_everything_leavesUsableTree);
    RUN_TEST(test_erase_nullArguments_doNotCrash);

    RUN_TEST(test_reposition_renamedContact_movesToNewPlace);
    RUN_TEST(test_reposition_keepsContactData);
    RUN_TEST(test_reposition_unchangedKey_keepsOrder);
    RUN_TEST(test_reposition_nullArguments_returnNull);

    RUN_TEST(test_height_emptyAndSingle);
    RUN_TEST(test_height_ascendingInsertion_degenerates);
    RUN_TEST(test_idealHeight_isLogOfSize);

    RUN_TEST(test_balance_degenerateTree_becomesShallow);
    RUN_TEST(test_balance_reusesTheSameNodes);
    RUN_TEST(test_balance_resetsCounter);
    RUN_TEST(test_balance_tooSmallTree_reportsNothingToDo);
    RUN_TEST(test_balance_nullArgument_doesNotCrash);

    RUN_TEST(test_autoBalance_withinThreshold_doesNotRebuild);
    RUN_TEST(test_autoBalance_degenerateTree_rebuildsItself);
    RUN_TEST(test_autoBalance_keepsAllContacts);

    RUN_TEST(test_fillTestData_addsAllTestContacts);
    RUN_TEST(test_fillTestData_everyContactIsComplete);

    RUN_TEST(test_interface_zero_exitsImmediately);
    RUN_TEST(test_interface_treeItem_showsHeightAndShape);
    RUN_TEST(test_interface_balanceItem_reportsNewHeight);
    RUN_TEST(test_interface_printItem_listsTestContacts);

    remove(STDIN_FIXTURE);
    remove(STDOUT_CAPTURE);
    return UNITY_END();
}
