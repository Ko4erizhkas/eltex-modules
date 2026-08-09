#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#include <stdio.h>

/*
 * Тесты меню и автобалансировки перехватывают stdout целиком
 * (см. captureStdout в test_phoneBook.c). Чтобы отчёт Unity при этом
 * не попал в файл перехвата, уводим её вывод в stderr.
 */
#define UNITY_OUTPUT_CHAR(a)  fputc((a), stderr)
#define UNITY_OUTPUT_FLUSH()  fflush(stderr)

#endif
