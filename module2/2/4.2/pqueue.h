#ifndef PQUEUE_H
#define PQUEUE_H

#include <stddef.h>

#define MAX_TEXT 64
#define PRIORITY_MIN 0
#define PRIORITY_MAX 255

typedef struct Message
{
    char text[MAX_TEXT];
    int priority;

} Message;

typedef struct PQNode
{
    Message data;
    struct PQNode* next;

} PQNode;

typedef struct PQueue
{
    PQNode* head;
    size_t size;

} PQueue;

PQueue* pq_init(void);
void pq_free(PQueue** pq);

int pq_push(PQueue* pq, const char* text, int priority);

int pq_pop(PQueue* pq, Message* out);
int pq_pop_priority(PQueue* pq, int priority, Message* out);
int pq_pop_atleast(PQueue* pq, int priority, Message* out);

void pq_print(const PQueue* pq);

#endif
