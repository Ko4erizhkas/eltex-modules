/*
 * Юнит-тесты для ipnet.c (задание 3.2)
 *
 * Все функции модуля - чистые преобразования над числом, ввода-вывода в них
 * нет, поэтому подменять stdin/stdout не требуется. ipnet.c линкуется с
 * тестом как обычный объектный файл; main() берётся отсюда, main.c в сборку
 * тестов не входит.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h> /* SetConsoleOutputCP, CP_UTF8 */
#endif

#include "unity/unity.h"
#include "../ipnet.h"

/* ------------------------------------------------------------------ */
/* Вспомогательные средства                                            */
/* ------------------------------------------------------------------ */

static unsigned parse(const char* str, int expectOk)
{
    unsigned addr = 0xDEADBEEF;

    TEST_ASSERT_EQUAL_INT_MESSAGE(expectOk, ip_parse(str, &addr), str);

    return addr;
}

static const char* format(unsigned addr)
{
    static char buffer[16];
    ip_format(addr, buffer);
    return buffer;
}

/* Маска задаётся строкой - так тесты читаются как ввод пользователя. */
static int maskValid(const char* str)
{
    unsigned mask = 0;

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ip_parse(str, &mask), str);

    return mask_is_valid(mask);
}

static int sameSubnet(const char* a, const char* b, const char* maskStr)
{
    unsigned first = 0, second = 0, mask = 0;

    TEST_ASSERT_EQUAL_INT(1, ip_parse(a, &first));
    TEST_ASSERT_EQUAL_INT(1, ip_parse(b, &second));
    TEST_ASSERT_EQUAL_INT(1, ip_parse(maskStr, &mask));

    return ip_same_subnet(first, second, mask);
}

void setUp(void)
{
}

void tearDown(void)
{
}

/* ------------------------------------------------------------------ */
/* ip_parse                                                            */
/* ------------------------------------------------------------------ */

static void test_parse_buildsNumberFromOctets(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xC0A80101, parse("192.168.1.1", 1));
    TEST_ASSERT_EQUAL_HEX32(0x0A000001, parse("10.0.0.1", 1));
}

static void test_parse_boundaryAddresses(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x00000000, parse("0.0.0.0", 1));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, parse("255.255.255.255", 1));
}

/* Старший октет больше 127 не должен превращаться в отрицательное число. */
static void test_parse_highOctetStaysUnsigned(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xC0000000, parse("192.0.0.0", 1));
}

static void test_parse_rejectsOctetOverRange(void)
{
    parse("256.0.0.1", 0);
    parse("192.168.1.300", 0);
}

static void test_parse_rejectsWrongOctetCount(void)
{
    parse("192.168.1", 0);
    parse("192.168.1.1.1", 0);
    parse("192.168.1.", 0);
}

static void test_parse_rejectsGarbage(void)
{
    parse("", 0);
    parse("abc", 0);
    parse("192.168.1.1x", 0);
    parse("192.168.1.1 1", 0);
}

/* ------------------------------------------------------------------ */
/* ip_format                                                           */
/* ------------------------------------------------------------------ */

static void test_format_commonAddresses(void)
{
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", format(0xC0A80101));
    TEST_ASSERT_EQUAL_STRING("0.0.0.0", format(0x00000000));
    TEST_ASSERT_EQUAL_STRING("255.255.255.255", format(0xFFFFFFFF));
}

static void test_format_isReverseOfParse(void)
{
    static const char* addresses[] = {
        "1.2.3.4", "10.0.0.1", "172.16.254.1", "255.0.128.7"
    };
    size_t i;

    for (i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++)
        TEST_ASSERT_EQUAL_STRING(addresses[i], format(parse(addresses[i], 1)));
}

/* ------------------------------------------------------------------ */
/* mask_is_valid                                                       */
/* ------------------------------------------------------------------ */

static void test_maskIsValid_acceptsContinuousOnes(void)
{
    TEST_ASSERT_EQUAL_INT(1, maskValid("0.0.0.0"));
    TEST_ASSERT_EQUAL_INT(1, maskValid("128.0.0.0"));
    TEST_ASSERT_EQUAL_INT(1, maskValid("255.0.0.0"));
    TEST_ASSERT_EQUAL_INT(1, maskValid("255.255.0.0"));
    TEST_ASSERT_EQUAL_INT(1, maskValid("255.255.255.0"));
    TEST_ASSERT_EQUAL_INT(1, maskValid("255.255.255.252"));
    TEST_ASSERT_EQUAL_INT(1, maskValid("255.255.255.255"));
}

static void test_maskIsValid_rejectsHolesAndTrailingOnes(void)
{
    TEST_ASSERT_EQUAL_INT(0, maskValid("255.0.255.0"));
    TEST_ASSERT_EQUAL_INT(0, maskValid("255.255.0.255"));
    TEST_ASSERT_EQUAL_INT(0, maskValid("255.255.255.1"));
    TEST_ASSERT_EQUAL_INT(0, maskValid("0.0.0.1"));
    TEST_ASSERT_EQUAL_INT(0, maskValid("0.255.255.255"));
}

/* ------------------------------------------------------------------ */
/* ip_same_subnet                                                      */
/* ------------------------------------------------------------------ */

static void test_sameSubnet_hostsBehindOneMask(void)
{
    TEST_ASSERT_EQUAL_INT(1, sameSubnet("192.168.1.1", "192.168.1.200", "255.255.255.0"));
    TEST_ASSERT_EQUAL_INT(0, sameSubnet("192.168.1.1", "192.168.2.200", "255.255.255.0"));
}

/* Те же адреса при более короткой маске оказываются в одной сети. */
static void test_sameSubnet_widerMaskJoinsNetworks(void)
{
    TEST_ASSERT_EQUAL_INT(1, sameSubnet("192.168.1.1", "192.168.2.200", "255.255.0.0"));
}

static void test_sameSubnet_edgeMasks(void)
{
    TEST_ASSERT_EQUAL_INT(1, sameSubnet("1.2.3.4", "200.100.50.25", "0.0.0.0"));
    TEST_ASSERT_EQUAL_INT(1, sameSubnet("1.2.3.4", "1.2.3.4", "255.255.255.255"));
    TEST_ASSERT_EQUAL_INT(0, sameSubnet("1.2.3.4", "1.2.3.5", "255.255.255.255"));
}

static void test_sameSubnet_gatewayItselfBelongsToSubnet(void)
{
    TEST_ASSERT_EQUAL_INT(1, sameSubnet("10.0.0.1", "10.0.0.1", "255.255.255.0"));
}

/* ------------------------------------------------------------------ */
/* ip_random                                                           */
/* ------------------------------------------------------------------ */

static void test_random_doesNotRepeatOneValue(void)
{
    unsigned first = ip_random();
    int different = 0;
    int i;

    for (i = 0; i < 100; i++)
    {
        if (ip_random() != first)
            different = 1;
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, different, "ip_random returns the same value");
}

/*
 * RAND_MAX может быть равен 32767, поэтому одного вызова rand() на адрес не
 * хватило бы: старшие биты всегда оставались бы нулевыми. Проверяем, что за
 * серию вызовов встречается единица в каждом из 32 бит.
 */
static void test_random_coversAllThirtyTwoBits(void)
{
    unsigned bits = 0;
    int i;

    for (i = 0; i < 1000; i++)
        bits |= ip_random();

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, bits);
}

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    UNITY_BEGIN();

    RUN_TEST(test_parse_buildsNumberFromOctets);
    RUN_TEST(test_parse_boundaryAddresses);
    RUN_TEST(test_parse_highOctetStaysUnsigned);
    RUN_TEST(test_parse_rejectsOctetOverRange);
    RUN_TEST(test_parse_rejectsWrongOctetCount);
    RUN_TEST(test_parse_rejectsGarbage);

    RUN_TEST(test_format_commonAddresses);
    RUN_TEST(test_format_isReverseOfParse);

    RUN_TEST(test_maskIsValid_acceptsContinuousOnes);
    RUN_TEST(test_maskIsValid_rejectsHolesAndTrailingOnes);

    RUN_TEST(test_sameSubnet_hostsBehindOneMask);
    RUN_TEST(test_sameSubnet_widerMaskJoinsNetworks);
    RUN_TEST(test_sameSubnet_edgeMasks);
    RUN_TEST(test_sameSubnet_gatewayItselfBelongsToSubnet);

    RUN_TEST(test_random_doesNotRepeatOneValue);
    RUN_TEST(test_random_coversAllThirtyTwoBits);

    return UNITY_END();
}
