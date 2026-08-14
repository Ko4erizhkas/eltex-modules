#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pqueue.h"

#define MESSAGE_COUNT 10

static const char* SOURCES[] = {
    "Датчик температуры",
    "Сетевой интерфейс",
    "Блок питания",
    "Пользовательский запрос",
    "Служба журналирования"
};

static void show(const char* title, int found, const Message* msg)
{
    if (found)
        printf("%s: приоритет %3d | %s\n", title, msg->priority, msg->text);
    else
        printf("%s: подходящих сообщений нет\n", title);
}

int main(void)
{
    PQueue* pq;
    Message msg;
    char text[MAX_TEXT];
    int i, priority;
    int sample = 0;

#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    pq = pq_init();
    if (!pq)
    {
        printf("Ошибка: не удалось создать очередь\n");
        return 1;
    }

    srand((unsigned)time(NULL));

    printf("Генерация %d сообщений:\n", MESSAGE_COUNT);
    for (i = 0; i < MESSAGE_COUNT; i++)
    {
        priority = rand() % (PRIORITY_MAX + 1);
        snprintf(text, MAX_TEXT, "%s #%d", SOURCES[rand() % 5], i + 1);
        printf("   приоритет %3d | %s\n", priority, text);

        if (!pq_push(pq, text, priority))
        {
            printf("Ошибка: не удалось добавить сообщение\n");
            pq_free(&pq);
            return 1;
        }

        if (i == MESSAGE_COUNT / 2)
            sample = priority;
    }

    printf("\nОчередь (%zu шт.):\n", pq->size);
    pq_print(pq);

    printf("\nВыборка:\n");
    show("Первое в очереди", pq_pop(pq, &msg), &msg);

    snprintf(text, MAX_TEXT, "С приоритетом %d", sample);
    show(text, pq_pop_priority(pq, sample, &msg), &msg);

    show("С приоритетом 300", pq_pop_priority(pq, 300, &msg), &msg);
    show("С приоритетом не ниже 200", pq_pop_atleast(pq, 200, &msg), &msg);
    show("С приоритетом не ниже 0", pq_pop_atleast(pq, 0, &msg), &msg);

    printf("\nОсталось в очереди (%zu шт.):\n", pq->size);
    pq_print(pq);

    pq_free(&pq);
    return 0;
}
