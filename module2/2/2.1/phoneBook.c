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
    free(pb->contacts[index]);
    
    memmove(&pb->contacts[index], &pb->contacts[index + 1], 
            (pb->countContacts - index - 1) * sizeof(Contact*));
    
    pb->contacts[pb->countContacts - 1] = NULL;
    pb->countContacts--;
}

static void deleteNumbers(Contact* c, int index)
{
    if (index < 0) 
    {
        fprintf(stderr, "Отрицательный индекс номера. Удаление невозможно!\n");
        return;
    }

    memmove(&c->phonenumber[index], &c->phonenumber[index + 1], 
        (c->countNumbers - index - 1)* sizeof(Numbers));
    
    c->phonenumber[c->countNumbers - 1].number[0] = '\0';
    c->countNumbers--;
}

static void deleteSocial(Contact* c, int index)
{
    if (index < 0) 
    {
        fprintf(stderr, "Отрицательный индекс соц сети. Удаление невозможно!\n");
        return;
    }

    memmove(&c->social[index], &c->social[index + 1],
    (c->countSocial - index - 1) * sizeof(Social));
    
    c->social[c->countSocial - 1].link[0] = '\0';
    c->social[c->countNumbers - 1].typeSocial[0] = '\0';
    c->social[c->countNumbers - 1].username[0] = '\0';
    c->countSocial--;

}
static void printContact(Contact* c)
{
    printf("Информация о контакте: \nФамилия: %s\nИмя: %s\nОтчество: %s\n", c->surname, c->name, c->patronymic);
    printf("Социальные сети:");
    if (c->countSocial == 0)
    {
        printf("-\n");
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
    printf("Номера телефонов: ");
    if(c->countNumbers == 0)
    {
        printf("-\n");
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
            printf("\n-----------------------\n");
            printf("Контакт #%d\n", i + 1);
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
void print_interface(void)
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    Phonebook* pb;
    pb = malloc(sizeof(Phonebook));

    if (pb == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для телефонной книги\n");
        return;
    }
    pb->countContacts = 0;

    int running = 1;
    while (running)
    {
        printf("\n%d. Добавить контакт\n", MENU_ADD);
        printf("%d. Удалить контакт\n", MENU_DELETE);
        printf("%d. Изменить контакт\n", MENU_EDIT);
        printf("%d. Вывести все контакты\n", MENU_PRINT);
        printf("%d. Выйти\n", MENU_EXIT);

        printf("Выберите действие: ");
        char line[8];
        fgets(line, sizeof(line), stdin);
        line[strcspn(line, "\n")] = '\0';
        int choice = atoi(line);

        switch((MenuAction)choice)
        {   
            case MENU_EXIT:
                running = 0;
                break;
            case MENU_ADD:
            {
                Contact *c = addContact();
                if (c != NULL) addContactInPhonebook(pb,c);
                break;
            }
            case MENU_DELETE:
            {
                printf("\n-------------------\n");
                printf("Какой контакт хотите удалить?: ");
                
                char line2[8];
                fgets(line2, sizeof(line2), stdin);
                line2[strcspn(line2, "\n")] = '\0';
                int delChoice = atoi(line2);
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
            case MENU_EDIT:
            {

                printf("\n-------------------\n");
                printf("Список контактов: \n");
                printPhonebook(pb);

                printf("\n-------------------\n");
                printf("Выберите контакт: ");
                
                char lineC[8];
                fgets(lineC, sizeof(lineC), stdin);
                lineC[strcspn(lineC, "\n")] = '\0';
                int contactNum = atoi(lineC);
                
                if (contactNum < 1 || contactNum > pb->countContacts)
                {
                    fprintf(stderr, "Некорректный номер контакта. Попробуйте снова\n");
                    break;
                }

                Contact* c = pb->contacts[contactNum - 1];
                printf("\nСписок доступных изменений:\n");
                printf("%d. Изменить Ф.И.О\n", EDIT_NAMES);
                printf("%d. Изменить соц сети\n", EDIT_SOCIAL);
                printf("%d. Удалить соц сети\n", EDIT_SOCIAL_DELETE);
                printf("%d. Изменить номер телефона\n", EDIT_PHONE);
                printf("%d. Удалить номер телефона\n", EDIT_PHONE_DELETE);

                printf("Выберите действие ");
                char line3[8];
                fgets(line3, sizeof(line3), stdin);
                line3[strcspn(line3, "\n")] = '\0';
                int choice3 = atoi(line3);

                switch((EditAction)choice3)
                {
                    case EDIT_EXIT: break;
                    case EDIT_NAMES:
                    {
                       // editNames(c);
                        break;
                    }
                    case EDIT_SOCIAL:
                    {
                        break;
                    }
                    case EDIT_SOCIAL_DELETE:
                    {
                        printf("Выберите номер соц сети: ");
                        char line3_3[8];
                        fgets(line3_3, sizeof(line3_3), stdin);
                        line3_3[strcspn(line3_3, "\n")] = '\0';
                        int socialNum = atoi(line3_3);
                        if(socialNum < 1)
                        {
                            fprintf(stderr, "Некорректный номер соц сети\n");
                            break;
                        }
                        deleteSocial(c, socialNum - 1);
                        break;
                    }
                    case EDIT_PHONE:
                    {
                        break;
                    }
                    case EDIT_PHONE_DELETE:
                    {
                        break;
                    }
                    default: 
                    {
                        fprintf(stderr, "Неизвестное действие\n");
                        break;
                    }
                }
                break;
            }
            case MENU_PRINT:
            {
                printPhonebook(pb);
                break;
            }
            default: 
            {
                fprintf(stderr, "Неизвестное дейтсвие\n");
                break;
            }
        }
    }
    for (int i = 0; i < pb->countContacts; i++)
    {
        free(pb->contacts[i]);
    }
    free(pb);
                
}







