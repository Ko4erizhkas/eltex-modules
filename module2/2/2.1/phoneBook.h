#pragma once
#define MAX_SIZE 128
typedef struct Person
{
    char firstName[MAX_SIZE];
    char lastName[MAX_SIZE];
    char surname[MAX_SIZE];
    char emailAddress[MAX_SIZE];
    char phoneNumber[MAX_SIZE];
    char media[MAX_SIZE];
} Person;
void addPerson();

struct PhoneBook
{
    Person data;
    struct PhoneBook* next;
};

void editPerson(Person p);
void deletePerson(Person p);