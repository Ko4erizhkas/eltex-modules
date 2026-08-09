#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stddef.h>

#define MAX_SIZE 128
#define MAX_SIZE_LINK 512
#define MAX_COUNT 4
#define MAX_COUNT_CONTACTS 8

typedef enum
{
    MENU_EXIT = 0,
    MENU_ADD = 1,
    MENU_DELETE = 2,
    MENU_EDIT = 3,
    MENU_PRINT = 4
} MenuAction;

typedef enum
{
    EDIT_EXIT = 0,
    EDIT_NAMES = 1,
    EDIT_SOCIAL = 2,
    EDIT_SOCIAL_DELETE = 3,
    EDIT_PHONE = 4,
    EDIT_PHONE_DELETE = 5
} EditAction;

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

void print_interface(void);

#endif
