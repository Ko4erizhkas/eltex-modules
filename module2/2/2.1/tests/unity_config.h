#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#include <stdio.h>

/*
 * Тестируемые функции печатают приглашения ввода в stdout, и тест глушит его
 * целиком (см. muteStdout в test_phoneBook.c). Чтобы отчёт Unity при этом не
 * пропал вместе с ним, уводим её вывод в stderr.
 */
#define UNITY_OUTPUT_CHAR(a)  fputc((a), stderr)
#define UNITY_OUTPUT_FLUSH()  fflush(stderr)

#endif
