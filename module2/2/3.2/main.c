#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ipnet.h"

int main(int argc, char* argv[])
{
    unsigned gateway, mask, dest;
    long count, i;
    long local = 0;
    char* end;
    char buf[16];

#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc != 4)
    {
        printf("Использование: %s <IP шлюза> <маска подсети> <количество пакетов>\n", argv[0]);
        printf("Пример: %s 192.168.1.1 255.255.255.0 100\n", argv[0]);
        return 1;
    }

    if (!ip_parse(argv[1], &gateway))
    {
        printf("Ошибка: некорректный IP адрес шлюза\n");
        return 1;
    }

    if (!ip_parse(argv[2], &mask) || !mask_is_valid(mask))
    {
        printf("Ошибка: некорректная маска подсети\n");
        return 1;
    }

    count = strtol(argv[3], &end, 10);
    if (*end != '\0' || count <= 0)
    {
        printf("Ошибка: количество пакетов должно быть целым числом больше нуля\n");
        return 1;
    }

    ip_format(gateway & mask, buf);
    printf("Своя подсеть: %s\n\n", buf);

    srand((unsigned)time(NULL));

    for (i = 0; i < count; i++)
    {
        dest = ip_random();
        ip_format(dest, buf);

        if (ip_same_subnet(dest, gateway, mask))
        {
            local++;
            printf("Пакет %ld: %-15s - своя подсеть\n", i + 1, buf);
        }
        else
        {
            printf("Пакет %ld: %-15s - другая сеть (через шлюз)\n", i + 1, buf);
        }
    }

    printf("\nВсего пакетов: %ld\n", count);
    printf("Своя подсеть:  %ld шт. (%.2f%%)\n", local, 100.0 * local / count);
    printf("Другие сети:   %ld шт. (%.2f%%)\n", count - local, 100.0 * (count - local) / count);

    return 0;
}
