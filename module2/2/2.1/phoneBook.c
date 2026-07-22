#include "phoneBook.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
void addPerson()
{
    printf("Введите имя: ");
    // scanf();
}

void initPerson(Person* p)
{
    p->firstName = malloc(MAX_SIZE * sizeof(char));
    p->lastName = malloc(MAX_SIZE * sizeof(char));
    p->surname = malloc(MAX_SIZE * sizeof(char));
}
void addEmailAddress(Person* p)
{
    p->emailAddress = malloc(MAX_SIZE * sizeof(char));
    
    char emailBuf[MAX_SIZE] = "";
    
    printf("Введите адрес электронной почты:");
    fflush(stdout);

    if (fgets(emailBuf, sizeof(emailBuf), stdin) == NULL) 
    {
        p->emailAddress = NULL;
    }

    emailBuf[strcspn(emailBuf, "\n")] = '\0';
    p->emailAddress = strdup(emailBuf);
}