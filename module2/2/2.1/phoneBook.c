#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

#include "phoneBook.h"



static void addNumber(Contact* c)
{
    if (c == NULL) 
    {
        fprintf(stderr, "Невозможно добавить номер в неинициализированный контакт\n");
        return;
    }
    c->countNumbers = 0;

    while (c->countNumbers < MAX_COUNT)
    {
        printf("Введите номер телефона #%d (пусто - завершить) (максимум 4 номера): ", c->countNumbers + 1);
        char* dest = c->phonenumber[c->countNumbers].number;
        size_t size = sizeof(c->phonenumber[c->countNumbers].number);

        if(fgets(dest, size, stdin) == NULL)
        {
            break;
        }
        dest[strcspn(dest,"\n")] = '\0';

        if (dest[0] == '\0')
        {
            break;
        }

        c->countNumbers++;
    }
}
static void addSocial(Contact* c)
{
    c->countSocial = 0;


    while (c->countSocial < MAX_COUNT)
    {
        Social* s = &c->social[c->countSocial];
        printf("Соцсеть #%d - платформа (пусто - завершить): ", c->countSocial + 1);

        fgets(s->typeSocial, sizeof(s->typeSocial), stdin);
        s->typeSocial[strcspn(s->typeSocial, "\n")] = '\0';

        if(s->typeSocial[0] == '\0') break;
        
        printf("\n");
        printf("Имя пользователя - обязательно: ");
        if(fgets(s->username, sizeof(s->username), stdin) == NULL) break;
        s->username[strcspn(s->username, "\n")] = '\0';
 
        printf("\n");
        printf("Ссылка на профиль - обязательно: ");
        if(fgets(s->link, sizeof(s->link), stdin) == NULL) break;
        s->link[strcspn(s->link, "\n")] = '\0';
        
        c->countSocial++;
    }
}
static void addNames(Contact* c)
{
    printf("\nВведите Ф.И.О\n");
    printf("Фамилия - обязательно: ");
    if(fgets(c->surname, sizeof(c->surname), stdin) == NULL) return;
    c->surname[strcspn(c->surname, "\n")] = '\0';
    if(c->surname[0] == '\0' || c->surname[0] == '\n')
    {
        printf("\nВведите Ф.И.О\n");
        printf("Фамилия - обязательно: ");
        if(fgets(c->surname, sizeof(c->surname), stdin) == NULL) return;
        c->surname[strcspn(c->surname, "\n")] = '\0';        
    }


    printf("\n");
    printf("Имя - обязательно: ");
    if(fgets(c->name, sizeof(c->name), stdin) == NULL) return;
    c->name[strcspn(c->name, "\n")] = '\0';
    if (c->name[0] == '\0' || c->name[0] == '\n')
    {
        printf("\n");
        printf("Имя - обязательно: ");
        if(fgets(c->name, sizeof(c->name), stdin) == NULL) return;
    }

    printf("\n");
    printf("Отчество: ");
    fgets(c->patronymic, sizeof(c->patronymic), stdin);
    c->patronymic[strcspn(c->patronymic, "\n")] = '\0';
    
    if (c->patronymic[0] == '\0')
    {
        strcpy(c->patronymic, "-");
    }
}
static Contact* addContact(void)
{
    Contact* c = malloc(sizeof(Contact));
    if (c == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для контакта");
        return NULL;
    }
    addNames(c);
    addNumber(c);
    addSocial(c);

    return c;
}

static void deleteContact(Phonebook* pb, int index)
{

}


static void printContact(Contact* c)
{
    printf("\n-----------------------\n");
    printf("Информация о контакте: \nФамилия: %s\nИмя: %s\nОтчество: %s\n", c->name, c->surname, c->patronymic);
    printf("Социальные сети:\n");
    if (c->countSocial == 0)
    {
        printf("Пусто\n");
    }
    else 
    {
        for (int i = 0; i < c->countSocial; ++i)
        {
            printf("Тип соцсети: %s\nСсылка на профиль: %s\nНикнейм: %s\n", 
                c->social[i].typeSocial,
                c->social[i].link,
                c->social[i].username);
        }
    }
    printf("Номера телефонов:\n");
    if(c->countNumbers == 0)
    {
        printf("Пусто\n");
    }
    else
    {
        for (int i = 0; i < c->countNumbers; ++i)
        {
            printf("Номер #%d  %s\n", i + 1, c->phonenumber[i].number);
        }
    }
}
static void printPhonebook(Phonebook* pb)
{
    for (int i = 0; i < pb->countContacts; i++)
    {
        if (pb->contacts[i] != NULL)
        {
            printContact(pb->contacts[i]);
        }
    }
}

static void addContactInPhonebook(Phonebook* pb, Contact* c)
{
    if (pb->countContacts >= MAX_COUNT_CONTACTS)
    {
        fprintf(stderr,"Телефонная книга заполнена\n");
        return;
    }
    pb->contacts[pb->countContacts] = c;
    pb->countContacts++;
}
void printInterface(void)
{
    Phonebook* pb;
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    pb = malloc(sizeof(Phonebook));
    
    if (pb == NULL) 
    {
        fprintf(stderr,"Ошибка выделения памяти для телефонной книги\n");
        return;
    }
    
    pb->countContacts = 0;
    
    for(;;)
    {


        printf("\n1. Добавить контакт\n");
        printf("2. Удалить контакт\n");
        printf("3. Изменить контакт\n");
        printf("4. Вывести все контакты\n");
        printf("0. Выйти\n");
        // printf("");


        printf("Выберите действие: ");

        char line[8];
        fgets(line, sizeof(line), stdin);
        line[strcspn(line, "\n")] = '\0';
        int choice = atoi(line);

        switch(choice)
        {
            case 0: break;
            case 1:
            {
                Contact* c = addContact();
                if (c != NULL) addContactInPhonebook(pb,c);
                break;
            }
            case 2:
            {
                int delChoice;
                printf("\nКакой контакт хотите удалить?: \n");
                scanf("%d\n", &delChoice);
                fflush(stdin);
                if (delChoice > 0 && delChoice <= pb->countContacts)
                {
                    deleteContact(pb, delChoice - 1);
                }
                else
                {
                    fprintf(stderr, "Некорректный номер контакта. Попробуйте снова\n");
                    continue;
                }
                break;
            }
            case 3:

                break;
            case 4:
                printPhonebook(pb);
                break;
            default: 
                fprintf(stderr, "Неизвестное действие");
                break;
        }

        if (choice == 0)
        {
            for (int i = 0; i < pb->countContacts; i++)
            {
                free(pb->contacts[i]);
            }
            free(pb);
            break;
        }
    }
}








