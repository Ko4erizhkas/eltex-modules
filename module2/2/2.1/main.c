#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>

#include "phoneBook.h"

int main()
{
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    Person p;
    //strncpy(p.firstName, "Ezh", 4);
    bool work = true;
    while (work)
    {
        size_t choice;

        printf("\n1. Add contact \n2. Edit contact \n3. Delete contact \n4. Close \n");
        printf("Select action: ");
        scanf("%zu", &choice);
        switch (choice)
        {
            case 1:
                size_t choice_1;
                printf(" ");
                scanf("%zu", &choice_1);
                addPerson(); 
                break;
            case 2: 
                break;
            case 3: 
                break; 
            case 4: 
                work = false;
                break;
            default:
                choice = 4; 
                break;
        }
    }
    return 0;
}