#include <stdio.h>
#include <stdlib.h>

#include "pqueue.h"

static int pq_extract(PQueue* pq, PQNode** link, Message* out)
{
    PQNode* node = *link;

    if (out)
        *out = node->data;

    *link = node->next;
    free(node);
    pq->size--;
    return 1;
}

PQueue* pq_init(void)
{
    PQueue* pq = malloc(sizeof(PQueue));
    if (!pq)
        return NULL;

    pq->head = NULL;
    pq->size = 0;
    return pq;
}

void pq_free(PQueue** pq)
{
    PQNode* node;
    PQNode* next;

    if (!pq || !*pq)
        return;

    node = (*pq)->head;
    while (node)
    {
        next = node->next;
        free(node);
        node = next;
    }

    free(*pq);
    *pq = NULL;
}

int pq_push(PQueue* pq, const char* text, int priority)
{
    PQNode* node;
    PQNode** link;

    if (!pq || !text || priority < PRIORITY_MIN || priority > PRIORITY_MAX)
        return 0;

    node = malloc(sizeof(PQNode));
    if (!node)
        return 0;

    snprintf(node->data.text, MAX_TEXT, "%s", text);
    node->data.priority = priority;

    link = &pq->head;
    while (*link && (*link)->data.priority >= priority)
        link = &(*link)->next;

    node->next = *link;
    *link = node;
    pq->size++;
    return 1;
}

int pq_pop(PQueue* pq, Message* out)
{
    if (!pq || !pq->head)
        return 0;

    return pq_extract(pq, &pq->head, out);
}

int pq_pop_priority(PQueue* pq, int priority, Message* out)
{
    PQNode** link;

    if (!pq)
        return 0;

    for (link = &pq->head; *link; link = &(*link)->next)
    {
        if ((*link)->data.priority == priority)
            return pq_extract(pq, link, out);
    }

    return 0;
}

int pq_pop_atleast(PQueue* pq, int priority, Message* out)
{
    PQNode** link;

    if (!pq)
        return 0;

    for (link = &pq->head; *link; link = &(*link)->next)
    {
        if ((*link)->data.priority >= priority)
            return pq_extract(pq, link, out);
    }

    return 0;
}

void pq_print(const PQueue* pq)
{
    const PQNode* node;
    size_t i = 1;

    if (!pq || !pq->head)
    {
        printf("Очередь пуста\n");
        return;
    }

    for (node = pq->head; node; node = node->next, i++)
        printf("%2zu) приоритет %3d | %s\n", i, node->data.priority, node->data.text);
}
