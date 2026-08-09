#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "phoneBook.h"


static void phlist_unlink(PhonebookList* phlist, PHNode* node);
static void phlist_link_sorted(PhonebookList* phlist, PHNode* node);


int contact_cmp(const Contact* a, const Contact* b)
{
    int result = strcmp(a->surname, b->surname);
    if (result != 0) return result;

    result = strcmp(a->name, b->name);
    if (result != 0) return result;

    return strcmp(a->patronymic, b->patronymic);
}

PhonebookList* phlist_init(void)
{
    PhonebookList* phlist = malloc(sizeof(PhonebookList));
    if (phlist == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для телефонной книги\n");
        return NULL;
    }

    phlist->head = NULL;
    phlist->tail = NULL;
    phlist->size = 0;

    return phlist;
}

void phlist_free(PhonebookList** phlist)
{
    if (phlist == NULL || *phlist == NULL) return;

    PHNode* node = (*phlist)->head;
    while (node != NULL)
    {
        PHNode* next = node->next;
        free(node);
        node = next;
    }

    free(*phlist);
    *phlist = NULL;
}

static void phlist_link_sorted(PhonebookList* phlist, PHNode* node)
{
    PHNode* pos = phlist->head;
    while (pos != NULL && contact_cmp(&pos->data, &node->data) <= 0)
    {
        pos = pos->next;
    }

    if (pos == NULL)
    {
        node->prev = phlist->tail;
        node->next = NULL;

        if (phlist->tail != NULL)
        {
            phlist->tail->next = node;
        }
        else
        {
            phlist->head = node;
        }

        phlist->tail = node;
    }
    else
    {
        node->prev = pos->prev;
        node->next = pos;

        if (pos->prev != NULL)
        {
            pos->prev->next = node;
        }
        else
        {
            phlist->head = node;
        }

        pos->prev = node;
    }

    phlist->size++;
}

static void phlist_unlink(PhonebookList* phlist, PHNode* node)
{
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        phlist->head = node->next;
    }

    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    else
    {
        phlist->tail = node->prev;
    }

    node->prev = NULL;
    node->next = NULL;
    phlist->size--;
}

PHNode* phlist_insert(PhonebookList* phlist, const Contact* c)
{
    if (phlist == NULL || c == NULL) return NULL;

    PHNode* node = malloc(sizeof(PHNode));
    if (node == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для контакта\n");
        return NULL;
    }

    node->data = *c;
    node->prev = NULL;
    node->next = NULL;

    phlist_link_sorted(phlist, node);

    return node;
}

void phlist_reposition(PhonebookList* phlist, PHNode* node)
{
    if (phlist == NULL || node == NULL) return;

    phlist_unlink(phlist, node);
    phlist_link_sorted(phlist, node);
}

PHNode* phlist_at(const PhonebookList* phlist, size_t index)
{
    if (phlist == NULL || index >= phlist->size) return NULL;

    PHNode* node = phlist->head;
    for (size_t i = 0; i < index; ++i)
    {
        node = node->next;
    }

    return node;
}

void phlist_erase(PhonebookList* phlist, PHNode* node)
{
    if (phlist == NULL || node == NULL) return;

    phlist_unlink(phlist, node);
    free(node);
}
