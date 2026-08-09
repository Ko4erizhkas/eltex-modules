#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stddef.h>

/* Интерфейс библиотеки libphonebook: данные контакта и двухсвязный
   упорядоченный список. Меню и ввод-вывод сюда не входят. */

#define MAX_SIZE 128
#define MAX_SIZE_LINK 512
#define MAX_COUNT 4

typedef struct Social
{
    char typeSocial[MAX_SIZE];
    char username[MAX_SIZE];
    char link[MAX_SIZE_LINK];

} Social;

typedef struct Numbers
{
    char number[MAX_SIZE];

} Numbers;

typedef struct Contact
{
    char name[MAX_SIZE];
    char surname[MAX_SIZE];
    char patronymic[MAX_SIZE];

    int countNumbers;
    int countSocial;

    Numbers phonenumber[MAX_COUNT];
    Social social[MAX_COUNT];

} Contact;

typedef struct PHNode
{
    Contact data;
    struct PHNode* prev;
    struct PHNode* next;

} PHNode;

typedef struct PhonebookList
{
    PHNode* head;
    PHNode* tail;
    size_t size;

} PhonebookList;

int contact_cmp(const Contact* a, const Contact* b);

PhonebookList* phlist_init(void);
void phlist_free(PhonebookList** phlist);
PHNode* phlist_insert(PhonebookList* phlist, const Contact* c);
PHNode* phlist_at(const PhonebookList* phlist, size_t index);
void phlist_erase(PhonebookList* phlist, PHNode* node);
void phlist_reposition(PhonebookList* phlist, PHNode* node);

#endif
