#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stdio.h>

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
       
    size_t countNumbers;
    size_t countSocial;
    
    Numbers phonenumber[MAX_COUNT];
    Social social[MAX_COUNT];

} Contact;

typedef struct Phonebook
{
    int countContacts;
    Contact* contacts[MAX_COUNT_CONTACTS];

} Phonebook;

static void addNames(Contact* c);
static void addNumber(Contact* c);
static void addSocial(Contact* c);
static Contact* addContact(void);
static void addContactInPhonebook(Phonebook* pb, Contact* c);

static void deleteContact(Phonebook* pb, int index);
static void deleteNumber(Contact* c, int index);
static void deleteSocial(Contact* c, int index);

static void editNames(Contact* c);
static void editNumber(Contact* c, int index);
static void editSocial(Contact* c, int index);

static void printPhonebook(Phonebook* pb);
static void printContact(Contact* c);

void print_interface(void);


#endif