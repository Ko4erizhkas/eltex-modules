/*
 * Юнит-тесты для perm.c (задание 3.1)
 *
 * Весь разбор и вывод прав - чистые функции над числом, поэтому здесь их
 * можно проверять напрямую, без подмены ввода. Публичного API хватает:
 * perm.c линкуется с тестом как обычный объектный файл, включать .c не нужно.
 * main() берётся из этого файла, main.c в сборку тестов не входит.
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
#include "../perm.h"

#define STDIN_FIXTURE  "stdin_fixture.tmp"
#define STDOUT_CAPTURE "stdout_capture.tmp"
#define FILE_FIXTURE   "perm_fixture.tmp"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

/* Подменяет stdin содержимым строки - так тестируется print_interface. */
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
 * Меню печатает результат в stdout, поэтому его мало заглушить - нужно
 * прочитать. Перехватываем в файл, потом возвращаем дескриптор на место.
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
    static char buffer[8192];

    restoreStdout();

    FILE* f = fopen(STDOUT_CAPTURE, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "не удалось открыть файл перехвата stdout");

    size_t readBytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[readBytes] = '\0';
    fclose(f);

    return buffer;
}

/* Прогоняет меню на заданном вводе и отдаёт всё, что попало в stdout. */
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

static void assertNotContains(const char* haystack, const char* needle,
                              const char* message)
{
    TEST_ASSERT_NULL_MESSAGE(strstr(haystack, needle), message);
}

static const char* sym(unsigned mode)
{
    static char buffer[10];
    perm_to_symbolic(mode, buffer);
    return buffer;
}

static const char* oct(unsigned mode)
{
    static char buffer[5];
    perm_to_octal(mode, buffer);
    return buffer;
}

static const char* bin(unsigned mode)
{
    static char buffer[16];
    perm_to_binary(mode, buffer);
    return buffer;
}

/* Применяет chmod-выражение к правам и проверяет сам факт успеха/отказа. */
static unsigned applyChmod(unsigned mode, char type, const char* expr, int expectOk)
{
    Perm perm = { mode, type, 1 };

    TEST_ASSERT_EQUAL_INT_MESSAGE(expectOk, perm_chmod(expr, &perm), expr);

    return perm.mode;
}

static unsigned parseAny(const char* str, int expectOk)
{
    unsigned mode = 07777;

    TEST_ASSERT_EQUAL_INT_MESSAGE(expectOk, perm_parse_any(str, &mode), str);

    return mode;
}

void setUp(void)
{
}

void tearDown(void)
{
    restoreStdout();
}

/* ------------------------------------------------------------------ */
/* perm_to_symbolic                                                    */
/* ------------------------------------------------------------------ */

static void test_toSymbolic_commonModes(void)
{
    TEST_ASSERT_EQUAL_STRING("rw-r--r--", sym(0644));
    TEST_ASSERT_EQUAL_STRING("rwxr-xr-x", sym(0755));
    TEST_ASSERT_EQUAL_STRING("rwxrwxrwx", sym(0777));
    TEST_ASSERT_EQUAL_STRING("---------", sym(0));
}

static void test_toSymbolic_everyClassIsIndependent(void)
{
    TEST_ASSERT_EQUAL_STRING("r--------", sym(0400));
    TEST_ASSERT_EQUAL_STRING("-w-------", sym(0200));
    TEST_ASSERT_EQUAL_STRING("--x------", sym(0100));
    TEST_ASSERT_EQUAL_STRING("---r-----", sym(0040));
    TEST_ASSERT_EQUAL_STRING("------r--", sym(0004));
}

/* Строчная s/t - бит x стоит, заглавная S/T - не стоит. */
static void test_toSymbolic_setuidWithExec_isLowercaseS(void)
{
    TEST_ASSERT_EQUAL_STRING("rwsr-xr-x", sym(04755));
}

static void test_toSymbolic_setuidWithoutExec_isUppercaseS(void)
{
    TEST_ASSERT_EQUAL_STRING("rwSr--r--", sym(04644));
}

static void test_toSymbolic_setgidWithExec_isLowercaseS(void)
{
    TEST_ASSERT_EQUAL_STRING("rwxr-sr-x", sym(02755));
}

static void test_toSymbolic_setgidWithoutExec_isUppercaseS(void)
{
    TEST_ASSERT_EQUAL_STRING("rw-r-Sr--", sym(02644));
}

static void test_toSymbolic_stickyWithExec_isLowercaseT(void)
{
    TEST_ASSERT_EQUAL_STRING("rwxrwxrwt", sym(01777));
}

static void test_toSymbolic_stickyWithoutExec_isUppercaseT(void)
{
    TEST_ASSERT_EQUAL_STRING("rw-rw-rwT", sym(01666));
}

/* ------------------------------------------------------------------ */
/* perm_to_octal                                                       */
/* ------------------------------------------------------------------ */

static void test_toOctal_alwaysFourDigits(void)
{
    TEST_ASSERT_EQUAL_STRING("0644", oct(0644));
    TEST_ASSERT_EQUAL_STRING("0755", oct(0755));
    TEST_ASSERT_EQUAL_STRING("0000", oct(0));
}

static void test_toOctal_specialBitsGoToFirstDigit(void)
{
    TEST_ASSERT_EQUAL_STRING("4755", oct(04755));
    TEST_ASSERT_EQUAL_STRING("2755", oct(02755));
    TEST_ASSERT_EQUAL_STRING("1777", oct(01777));
    TEST_ASSERT_EQUAL_STRING("7777", oct(07777));
}

/* ------------------------------------------------------------------ */
/* perm_to_binary                                                      */
/* ------------------------------------------------------------------ */

static void test_toBinary_twelveBitsInFourGroups(void)
{
    TEST_ASSERT_EQUAL_STRING("000 110 100 100", bin(0644));
    TEST_ASSERT_EQUAL_STRING("000 111 101 101", bin(0755));
    TEST_ASSERT_EQUAL_STRING("000 000 000 000", bin(0));
    TEST_ASSERT_EQUAL_STRING("111 111 111 111", bin(07777));
}

/* Ради этой тройки битов вывод и сделан двенадцатибитным:
   иначе 4755 и 0755 в двоичном виде выглядели бы одинаково. */
static void test_toBinary_specialBitsAreVisible(void)
{
    TEST_ASSERT_EQUAL_STRING("100 111 101 101", bin(04755));
    TEST_ASSERT_EQUAL_STRING("010 111 101 101", bin(02755));
    TEST_ASSERT_EQUAL_STRING("001 111 111 111", bin(01777));
}

/* ------------------------------------------------------------------ */
/* perm_parse_octal                                                    */
/* ------------------------------------------------------------------ */

static void test_parseOctal_threeAndFourDigits(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(1, perm_parse_octal("644", &mode));
    TEST_ASSERT_EQUAL_UINT(0644, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_octal("0755", &mode));
    TEST_ASSERT_EQUAL_UINT(0755, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_octal("4755", &mode));
    TEST_ASSERT_EQUAL_UINT(04755, mode);
}

static void test_parseOctal_shortForms(void)
{
    unsigned mode = 07777;

    TEST_ASSERT_EQUAL_INT(1, perm_parse_octal("0", &mode));
    TEST_ASSERT_EQUAL_UINT(0, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_octal("7", &mode));
    TEST_ASSERT_EQUAL_UINT(07, mode);
}

static void test_parseOctal_rejectsNonOctalDigits(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(0, perm_parse_octal("888", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_octal("64a", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_octal("-644", &mode));
}

static void test_parseOctal_rejectsEmptyAndTooLong(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(0, perm_parse_octal("", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_octal("07777", &mode));
}

/* ------------------------------------------------------------------ */
/* perm_parse_symbolic                                                 */
/* ------------------------------------------------------------------ */

static void test_parseSymbolic_nineCharacters(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("rw-r--r--", &mode));
    TEST_ASSERT_EQUAL_UINT(0644, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("rwxr-xr-x", &mode));
    TEST_ASSERT_EQUAL_UINT(0755, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("---------", &mode));
    TEST_ASSERT_EQUAL_UINT(0, mode);
}

/* Строку можно скопировать прямо из вывода ls -l вместе с типом файла. */
static void test_parseSymbolic_tenCharacters_skipsFileType(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("-rw-r--r--", &mode));
    TEST_ASSERT_EQUAL_UINT(0644, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("drwxr-xr-x", &mode));
    TEST_ASSERT_EQUAL_UINT(0755, mode);
}

static void test_parseSymbolic_specialLetters(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("rwsr-xr-x", &mode));
    TEST_ASSERT_EQUAL_UINT(04755, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("rwSr--r--", &mode));
    TEST_ASSERT_EQUAL_UINT(04644, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("rwxrwxrwt", &mode));
    TEST_ASSERT_EQUAL_UINT(01777, mode);

    TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic("rwxrwxrwT", &mode));
    TEST_ASSERT_EQUAL_UINT(01776, mode);
}

static void test_parseSymbolic_isReverseOfToSymbolic(void)
{
    const unsigned modes[] = { 0, 0644, 0755, 0777, 04755, 02644, 01777, 07777 };

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
    {
        unsigned back = 07777;
        TEST_ASSERT_EQUAL_INT(1, perm_parse_symbolic(sym(modes[i]), &back));
        TEST_ASSERT_EQUAL_UINT(modes[i], back);
    }
}

static void test_parseSymbolic_rejectsWrongLength(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(0, perm_parse_symbolic("rw-r--r", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_symbolic("rwxrwxrwxx", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_symbolic("", &mode));
}

static void test_parseSymbolic_rejectsWrongLetters(void)
{
    unsigned mode = 0;

    /* буква не на своей позиции */
    TEST_ASSERT_EQUAL_INT(0, perm_parse_symbolic("wr-r--r--", &mode));
    /* s в классе "остальные" - там может быть только t */
    TEST_ASSERT_EQUAL_INT(0, perm_parse_symbolic("rwxrwxrws", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_symbolic("zw-r--r--", &mode));
}

/* ------------------------------------------------------------------ */
/* perm_parse_any                                                      */
/* ------------------------------------------------------------------ */

static void test_parseAny_digitChoosesOctal(void)
{
    TEST_ASSERT_EQUAL_UINT(0644, parseAny("644", 1));
    TEST_ASSERT_EQUAL_UINT(04755, parseAny("4755", 1));
}

static void test_parseAny_letterChoosesSymbolic(void)
{
    TEST_ASSERT_EQUAL_UINT(0644, parseAny("rw-r--r--", 1));
    TEST_ASSERT_EQUAL_UINT(0755, parseAny("-rwxr-xr-x", 1));
}

static void test_parseAny_rejectsGarbage(void)
{
    unsigned mode = 0;

    TEST_ASSERT_EQUAL_INT(0, perm_parse_any("", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_any("hello", &mode));
    TEST_ASSERT_EQUAL_INT(0, perm_parse_any("9999", &mode));
}

/* ------------------------------------------------------------------ */
/* perm_chmod - числовая форма                                         */
/* ------------------------------------------------------------------ */

static void test_chmod_numeric_replacesEverything(void)
{
    TEST_ASSERT_EQUAL_UINT(0755, applyChmod(0644, '-', "755", 1));
    TEST_ASSERT_EQUAL_UINT(0, applyChmod(07777, '-', "0", 1));
    TEST_ASSERT_EQUAL_UINT(04755, applyChmod(0644, '-', "4755", 1));
}

/* ------------------------------------------------------------------ */
/* perm_chmod - плюс, минус, равно                                     */
/* ------------------------------------------------------------------ */

static void test_chmod_plus_addsBitsOnly(void)
{
    TEST_ASSERT_EQUAL_UINT(0744, applyChmod(0644, '-', "u+x", 1));
    TEST_ASSERT_EQUAL_UINT(0664, applyChmod(0644, '-', "g+w", 1));
    /* бит уже стоит - ничего не меняется */
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "u+r", 1));
}

static void test_chmod_minus_removesBitsOnly(void)
{
    TEST_ASSERT_EQUAL_UINT(0600, applyChmod(0644, '-', "go-r", 1));
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "go-x", 1));
    TEST_ASSERT_EQUAL_UINT(0044, applyChmod(0644, '-', "u-rw", 1));
}

static void test_chmod_equals_replacesClassEntirely(void)
{
    TEST_ASSERT_EQUAL_UINT(0666, applyChmod(0755, '-', "a=rw", 1));
    TEST_ASSERT_EQUAL_UINT(0055, applyChmod(0755, '-', "u=", 1));
    TEST_ASSERT_EQUAL_UINT(0, applyChmod(07777, '-', "a=", 1));
}

/* Пустой класс означает "все", как и у chmod */
static void test_chmod_omittedWho_meansAll(void)
{
    TEST_ASSERT_EQUAL_UINT(0755, applyChmod(0644, '-', "+x", 1));
    TEST_ASSERT_EQUAL_UINT(0200, applyChmod(0644, '-', "-r", 1));
}

static void test_chmod_severalClausesSeparatedByComma(void)
{
    TEST_ASSERT_EQUAL_UINT(0755, applyChmod(0, '-', "u=rwx,go=rx", 1));
    TEST_ASSERT_EQUAL_UINT(0700, applyChmod(0777, '-', "go-rwx", 1));
}

/* Несколько операторов в одной клаузе применяются слева направо */
static void test_chmod_severalOperatorsInOneClause(void)
{
    TEST_ASSERT_EQUAL_UINT(0500, applyChmod(0600, '-', "u+x-w", 1));
    TEST_ASSERT_EQUAL_UINT(0700, applyChmod(0000, '-', "u+r+w+x", 1));
}

/* ------------------------------------------------------------------ */
/* perm_chmod - специальные биты                                       */
/* ------------------------------------------------------------------ */

static void test_chmod_setuidAndSetgid(void)
{
    TEST_ASSERT_EQUAL_UINT(04755, applyChmod(0755, '-', "u+s", 1));
    TEST_ASSERT_EQUAL_UINT(02755, applyChmod(0755, '-', "g+s", 1));
    /* для "всех" s означает сразу setuid и setgid */
    TEST_ASSERT_EQUAL_UINT(06755, applyChmod(0755, '-', "a+s", 1));
    TEST_ASSERT_EQUAL_UINT(0755, applyChmod(06755, '-', "ug-s", 1));
}

static void test_chmod_stickyBit(void)
{
    TEST_ASSERT_EQUAL_UINT(01777, applyChmod(0777, 'd', "+t", 1));
    TEST_ASSERT_EQUAL_UINT(0777, applyChmod(01777, 'd', "-t", 1));
}

/* u= обязан снести и setuid, иначе бит остаётся висеть незаметно */
static void test_chmod_equalsClearsSpecialBitOfItsClass(void)
{
    TEST_ASSERT_EQUAL_UINT(0655, applyChmod(04755, '-', "u=rw", 1));
    TEST_ASSERT_EQUAL_UINT(0745, applyChmod(02755, '-', "g=r", 1));
}

/* ------------------------------------------------------------------ */
/* perm_chmod - копирование класса и X                                 */
/* ------------------------------------------------------------------ */

static void test_chmod_copiesRightsFromAnotherClass(void)
{
    TEST_ASSERT_EQUAL_UINT(0770, applyChmod(0700, '-', "g+u", 1));
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0640, '-', "o=g", 1));
}

/* X ставит x только там, где он уже где-то есть, либо у каталога */
static void test_chmod_bigX_onFileWithoutExec_addsNothing(void)
{
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "a+X", 1));
}

static void test_chmod_bigX_onFileWithExec_addsExec(void)
{
    TEST_ASSERT_EQUAL_UINT(0755, applyChmod(0744, '-', "go+X", 1));
}

static void test_chmod_bigX_onDirectory_alwaysAddsExec(void)
{
    TEST_ASSERT_EQUAL_UINT(0755, applyChmod(0644, 'd', "a+X", 1));
}

/* ------------------------------------------------------------------ */
/* perm_chmod - отказы                                                 */
/* ------------------------------------------------------------------ */

static void test_chmod_rejectsGarbage_andKeepsMode(void)
{
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "u?x", 0));
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "x", 0));
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "", 0));
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "u+x,", 0));
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "999", 0));
}

/* Клауза разобралась, но следом мусор - применять нельзя ничего */
static void test_chmod_partiallyValidExpression_changesNothing(void)
{
    TEST_ASSERT_EQUAL_UINT(0644, applyChmod(0644, '-', "u+x,g?w", 0));
}

/* ------------------------------------------------------------------ */
/* perm_from_file                                                      */
/* ------------------------------------------------------------------ */

static void test_fromFile_regularFile(void)
{
    FILE* f = fopen(FILE_FIXTURE, "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs("test", f);
    fclose(f);

    Perm perm = { 0, '?', 0 };
    TEST_ASSERT_EQUAL_INT(1, perm_from_file(FILE_FIXTURE, &perm));

    TEST_ASSERT_EQUAL_INT('-', perm.type);
    TEST_ASSERT_EQUAL_INT(1, perm.isSet);
    /* владелец обязан иметь чтение и запись на только что созданном файле */
    TEST_ASSERT_EQUAL_UINT(0600, perm.mode & 0600);

    remove(FILE_FIXTURE);
}

static void test_fromFile_directory(void)
{
    Perm perm = { 0, '?', 0 };

    TEST_ASSERT_EQUAL_INT(1, perm_from_file(".", &perm));
    TEST_ASSERT_EQUAL_INT('d', perm.type);
}

static void test_fromFile_missingFile_reportsFailure(void)
{
    Perm perm = { 0644, '-', 1 };

    TEST_ASSERT_EQUAL_INT(0, perm_from_file("no_such_file_here.tmp", &perm));
    /* права не должны быть затёрты неудачным чтением */
    TEST_ASSERT_EQUAL_UINT(0644, perm.mode);
}

static void test_fromFile_rejectsEmptyPathAndNull(void)
{
    Perm perm = { 0, '-', 0 };

    TEST_ASSERT_EQUAL_INT(0, perm_from_file("", &perm));
    TEST_ASSERT_EQUAL_INT(0, perm_from_file(NULL, &perm));
    TEST_ASSERT_EQUAL_INT(0, perm_from_file(FILE_FIXTURE, NULL));
}

/* ------------------------------------------------------------------ */
/* print_interface                                                     */
/* ------------------------------------------------------------------ */

static void test_interface_zero_exitsImmediately(void)
{
    const char* out = runInterface("0\n");

    assertContains(out, "Выберите действие", "menu prompt is expected");
    assertNotContains(out, "Буквенно", "no rights are expected to be printed");
}

static void test_interface_showsAllThreeRepresentations(void)
{
    const char* out = runInterface("1\n644\n0\n");

    assertContains(out, "rw-r--r--", "symbolic form is expected");
    assertContains(out, "0644", "octal form is expected");
    assertContains(out, "000 110 100 100", "binary form is expected");
}

static void test_interface_acceptsSymbolicInput(void)
{
    const char* out = runInterface("1\nrwxr-xr-x\n0\n");

    assertContains(out, "0755", "symbolic input is expected to be accepted");
}

static void test_interface_chmodChangesEnteredRights(void)
{
    const char* out = runInterface("1\n0755\n3\nu-w,go-rx\n0\n");

    assertContains(out, "0500", "u-w,go-rx applied to 0755 is expected to give 0500");
}

static void test_interface_badRightsAreRejected(void)
{
    const char* out = runInterface("1\nzzz\n0\n");

    assertNotContains(out, "Буквенно", "garbage input is expected to be rejected");
}

static void test_interface_readsRightsFromFile(void)
{
    FILE* f = fopen(FILE_FIXTURE, "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs("test", f);
    fclose(f);

    const char* out = runInterface("2\n" FILE_FIXTURE "\n0\n");

    assertContains(out, "Буквенно", "file rights are expected to be printed");
    assertContains(out, "rw-", "owner is expected to have read and write");

    remove(FILE_FIXTURE);
}

/* Ввод кончился - меню обязано завершиться, а не крутиться на EOF */
static void test_interface_eof_endsMenu(void)
{
    const char* out = runInterface("1\n644\n");

    assertContains(out, "0644", "rights are expected to be printed before EOF");
}

/* ------------------------------------------------------------------ */

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    UNITY_BEGIN();

    RUN_TEST(test_toSymbolic_commonModes);
    RUN_TEST(test_toSymbolic_everyClassIsIndependent);
    RUN_TEST(test_toSymbolic_setuidWithExec_isLowercaseS);
    RUN_TEST(test_toSymbolic_setuidWithoutExec_isUppercaseS);
    RUN_TEST(test_toSymbolic_setgidWithExec_isLowercaseS);
    RUN_TEST(test_toSymbolic_setgidWithoutExec_isUppercaseS);
    RUN_TEST(test_toSymbolic_stickyWithExec_isLowercaseT);
    RUN_TEST(test_toSymbolic_stickyWithoutExec_isUppercaseT);

    RUN_TEST(test_toOctal_alwaysFourDigits);
    RUN_TEST(test_toOctal_specialBitsGoToFirstDigit);

    RUN_TEST(test_toBinary_twelveBitsInFourGroups);
    RUN_TEST(test_toBinary_specialBitsAreVisible);

    RUN_TEST(test_parseOctal_threeAndFourDigits);
    RUN_TEST(test_parseOctal_shortForms);
    RUN_TEST(test_parseOctal_rejectsNonOctalDigits);
    RUN_TEST(test_parseOctal_rejectsEmptyAndTooLong);

    RUN_TEST(test_parseSymbolic_nineCharacters);
    RUN_TEST(test_parseSymbolic_tenCharacters_skipsFileType);
    RUN_TEST(test_parseSymbolic_specialLetters);
    RUN_TEST(test_parseSymbolic_isReverseOfToSymbolic);
    RUN_TEST(test_parseSymbolic_rejectsWrongLength);
    RUN_TEST(test_parseSymbolic_rejectsWrongLetters);

    RUN_TEST(test_parseAny_digitChoosesOctal);
    RUN_TEST(test_parseAny_letterChoosesSymbolic);
    RUN_TEST(test_parseAny_rejectsGarbage);

    RUN_TEST(test_chmod_numeric_replacesEverything);
    RUN_TEST(test_chmod_plus_addsBitsOnly);
    RUN_TEST(test_chmod_minus_removesBitsOnly);
    RUN_TEST(test_chmod_equals_replacesClassEntirely);
    RUN_TEST(test_chmod_omittedWho_meansAll);
    RUN_TEST(test_chmod_severalClausesSeparatedByComma);
    RUN_TEST(test_chmod_severalOperatorsInOneClause);

    RUN_TEST(test_chmod_setuidAndSetgid);
    RUN_TEST(test_chmod_stickyBit);
    RUN_TEST(test_chmod_equalsClearsSpecialBitOfItsClass);

    RUN_TEST(test_chmod_copiesRightsFromAnotherClass);
    RUN_TEST(test_chmod_bigX_onFileWithoutExec_addsNothing);
    RUN_TEST(test_chmod_bigX_onFileWithExec_addsExec);
    RUN_TEST(test_chmod_bigX_onDirectory_alwaysAddsExec);

    RUN_TEST(test_chmod_rejectsGarbage_andKeepsMode);
    RUN_TEST(test_chmod_partiallyValidExpression_changesNothing);

    RUN_TEST(test_fromFile_regularFile);
    RUN_TEST(test_fromFile_directory);
    RUN_TEST(test_fromFile_missingFile_reportsFailure);
    RUN_TEST(test_fromFile_rejectsEmptyPathAndNull);

    RUN_TEST(test_interface_zero_exitsImmediately);
    RUN_TEST(test_interface_showsAllThreeRepresentations);
    RUN_TEST(test_interface_acceptsSymbolicInput);
    RUN_TEST(test_interface_chmodChangesEnteredRights);
    RUN_TEST(test_interface_badRightsAreRejected);
    RUN_TEST(test_interface_readsRightsFromFile);
    RUN_TEST(test_interface_eof_endsMenu);

    remove(STDIN_FIXTURE);
    remove(STDOUT_CAPTURE);
    remove(FILE_FIXTURE);
    return UNITY_END();
}
