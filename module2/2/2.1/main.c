#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "phoneBook.h"

int main()
{
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
            case 0:
                break;
            case 1: 
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