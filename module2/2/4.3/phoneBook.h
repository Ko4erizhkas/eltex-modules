#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stddef.h>

#define MAX_SIZE 128
#define MAX_SIZE_LINK 512
#define MAX_COUNT 4
#define MAX_COUNT_CONTACTS 8

/* Через сколько добавлений/удалений проверять, не выродилось ли дерево */
#define BALANCE_PERIOD 8

/* Во сколько раз высота может превышать идеальную до перестроения */
#define BALANCE_FACTOR 2

typedef enum
{
    MENU_EXIT = 0,
    MENU_ADD = 1,
    MENU_DELETE = 2,
    MENU_EDIT = 3,
    MENU_PRINT = 4,
    MENU_TREE = 5,
    MENU_BALANCE = 6
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
    struct PHNode* parent;
    struct PHNode* left;
    struct PHNode* right;

} PHNode;

typedef struct PhonebookTree
{
    PHNode* root;
    size_t size;
    size_t opsSinceBalance; /* изменений с последней проверки баланса */

} PhonebookTree;

int contact_cmp(const Contact* a, const Contact* b);

PhonebookTree* phtree_init(void);
void phtree_free(PhonebookTree** phtree);
PHNode* phtree_insert(PhonebookTree* phtree, const Contact* c);
PHNode* phtree_at(const PhonebookTree* phtree, size_t index);
void phtree_erase(PhonebookTree* phtree, PHNode* node);
PHNode* phtree_reposition(PhonebookTree* phtree, PHNode* node);

int phtree_balance(PhonebookTree* phtree);
size_t phtree_height(const PhonebookTree* phtree);
size_t phtree_ideal_height(size_t size);

void print_interface(void);

#endif
