#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stdio.h>

#define MAX_SIZE 128
#define MAX_SIZE_LINK 512
#define MAX_COUNT 4
#define MAX_COUNT_CONTACTS 8

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

static void editContact(Phonebook* pb, int index);
static void editNumber(Contact* c, int index);
static void editSocial(Contact* c, int index);

static void printPhonebook(Phonebook* pb);
static void printContact(Contact* c);


void printInterface(void);



#endif