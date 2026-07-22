#pragma once
#define MAX_SIZE 128
typedef struct Person
{
    char* firstName;
    char* lastName;
    char* surname;
    char* emailAddress;
    char* phoneNumber;
    char* media;
} Person;

void initPerson(Person* p);
void addPerson();
struct Media
{
    
};





struct PhoneBook
{
    Person data;
    struct PhoneBook* next;
};

void editPerson(Person p);
void deletePerson(Person p);