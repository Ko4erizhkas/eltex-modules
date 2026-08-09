#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "phoneBook.h"


static int addNames(Contact* c);
static void addNumber(Contact* c);
static void addSocial(Contact* c);
static int addContact(Contact* c);

static void deleteNumber(Contact* c, int index);
static void deleteSocial(Contact* c, int index);

static void editNames(Contact* c);
static void editNumber(Contact* c, int index);
static void editSocial(Contact* c, int index);

static void printPhonebook(const PhonebookList* pb);
static void printContact(const Contact* c);

static void phlist_unlink(PhonebookList* phlist, PHNode* node);
static void phlist_link_sorted(PhonebookList* phlist, PHNode* node);


int contact_cmp(const Contact* a, const Contact* b)
{
    int result = strcmp(a->surname, b->surname);
    if (result != 0) return result;

    result = strcmp(a->name, b->name);
    if (result != 0) return result;

    return strcmp(a->patronymic, b->patronymic);
}

PhonebookList* phlist_init(void)
{
    PhonebookList* phlist = malloc(sizeof(PhonebookList));
    if (phlist == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для телефонной книги\n");
        return NULL;
    }

    phlist->head = NULL;
    phlist->tail = NULL;
    phlist->size = 0;

    return phlist;
}

void phlist_free(PhonebookList** phlist)
{
    if (phlist == NULL || *phlist == NULL) return;

    PHNode* node = (*phlist)->head;
    while (node != NULL)
    {
        PHNode* next = node->next;
        free(node);
        node = next;
    }

    free(*phlist);
    *phlist = NULL;
}

static void phlist_link_sorted(PhonebookList* phlist, PHNode* node)
{
    PHNode* pos = phlist->head;
    while (pos != NULL && contact_cmp(&pos->data, &node->data) <= 0)
    {
        pos = pos->next;
    }

    if (pos == NULL)
    {
        node->prev = phlist->tail;
        node->next = NULL;

        if (phlist->tail != NULL)
        {
            phlist->tail->next = node;
        }
        else
        {
            phlist->head = node;
        }

        phlist->tail = node;
    }
    else
    {
        node->prev = pos->prev;
        node->next = pos;

        if (pos->prev != NULL)
        {
            pos->prev->next = node;
        }
        else
        {
            phlist->head = node;
        }

        pos->prev = node;
    }

    phlist->size++;
}

static void phlist_unlink(PhonebookList* phlist, PHNode* node)
{
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        phlist->head = node->next;
    }

    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    else
    {
        phlist->tail = node->prev;
    }

    node->prev = NULL;
    node->next = NULL;
    phlist->size--;
}

PHNode* phlist_insert(PhonebookList* phlist, const Contact* c)
{
    if (phlist == NULL || c == NULL) return NULL;

    PHNode* node = malloc(sizeof(PHNode));
    if (node == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для контакта\n");
        return NULL;
    }

    node->data = *c;
    node->prev = NULL;
    node->next = NULL;

    phlist_link_sorted(phlist, node);

    return node;
}

void phlist_reposition(PhonebookList* phlist, PHNode* node)
{
    if (phlist == NULL || node == NULL) return;

    phlist_unlink(phlist, node);
    phlist_link_sorted(phlist, node);
}

PHNode* phlist_at(const PhonebookList* phlist, size_t index)
{
    if (phlist == NULL || index >= phlist->size) return NULL;

    PHNode* node = phlist->head;
    for (size_t i = 0; i < index; ++i)
    {
        node = node->next;
    }

    return node;
}

void phlist_erase(PhonebookList* phlist, PHNode* node)
{
    if (phlist == NULL || node == NULL) return;

    phlist_unlink(phlist, node);
    free(node);
}


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

        if(fgets(s->typeSocial, sizeof(s->typeSocial), stdin) == NULL) break;
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
static int addNames(Contact* c)
{
    printf("\nВведите Ф.И.О\n");
    printf("Фамилия - обязательно: ");
    if(fgets(c->surname, sizeof(c->surname), stdin) == NULL) return 0;
    c->surname[strcspn(c->surname, "\n")] = '\0';
    if(c->surname[0] == '\0')
    {
        printf("\nВведите Ф.И.О\n");
        printf("Фамилия - обязательно: ");
        if(fgets(c->surname, sizeof(c->surname), stdin) == NULL) return 0;
        c->surname[strcspn(c->surname, "\n")] = '\0';
    }


    printf("\n");
    printf("Имя - обязательно: ");
    if(fgets(c->name, sizeof(c->name), stdin) == NULL) return 0;
    c->name[strcspn(c->name, "\n")] = '\0';
    if (c->name[0] == '\0')
    {
        printf("\n");
        printf("Имя - обязательно: ");
        if(fgets(c->name, sizeof(c->name), stdin) == NULL) return 0;
        c->name[strcspn(c->name, "\n")] = '\0';
    }

    printf("\n");
    printf("Отчество: ");
    if(fgets(c->patronymic, sizeof(c->patronymic), stdin) == NULL) return 0;
    c->patronymic[strcspn(c->patronymic, "\n")] = '\0';

    if (c->patronymic[0] == '\0')
    {
        strcpy(c->patronymic, "-");
    }

    return 1;
}

static int addContact(Contact* c)
{
    if (c == NULL) return 0;

    memset(c, 0, sizeof(*c));

    if (!addNames(c)) return 0;
    addNumber(c);
    addSocial(c);

    return 1;
}


static void deleteSocial(Contact* c, int index)
{
    if (index < 0)
    {
        fprintf(stderr, "Отрицательный индекс соц сети. Удаление невозможно!\n");
        return;
    }

    if (index >= c->countSocial)
    {
        fprintf(stderr, "Выход за границы списка соц сетей\n");
        return;
    }

    memmove(&c->social[index], &c->social[index + 1],
            (c->countSocial - index - 1) * sizeof(Social));

    c->social[c->countSocial - 1].link[0] = '\0';
    c->social[c->countSocial - 1].typeSocial[0] = '\0';
    c->social[c->countSocial - 1].username[0] = '\0';
    c->countSocial--;

}
static void deleteNumber(Contact* c, int index)
{
    if (index < 0)
    {
        fprintf(stderr, "Отрицательный индекс номера. Удаление невозможно!\n");
        return;
    }

    if (index >= c->countNumbers)
    {
        fprintf(stderr, "Выход за границы списка номеров\n");
        return;
    }

    memmove(&c->phonenumber[index], &c->phonenumber[index + 1],
            (c->countNumbers - index - 1) * sizeof(Numbers));

    c->phonenumber[c->countNumbers - 1].number[0] = '\0';
    c->countNumbers--;
}


static void editNames(Contact* c)
{
    char buffer[MAX_SIZE];

    printf("\nИзмениние Ф.И.О (пусто - оставить без изменений)\n");

    printf("Имя [%s]: ", c->name);
    if(fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(buffer[0] != '\0')
        {
            strcpy(c->name, buffer);
        }
    }

    printf("Фамилия [%s]: ", c->surname);
    if(fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(buffer[0] != '\0')
        {
            strcpy(c->surname, buffer);
        }
    }

    printf("Отчество [%s]: ", c->patronymic);
    if(fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(buffer[0] != '\0')
        {
            strcpy(c->patronymic, buffer);
        }
    }
}
static void editNumber(Contact* c, int index)
{
    if (index < 0)
    {
        fprintf(stderr, "Отрицательный индекс номера. Изменение невозможно!\n");
        return;
    }

    if (index >= c->countNumbers)
    {
        fprintf(stderr, "Выход за границы списка номеров\n");
        return;
    }
    char buffer[MAX_SIZE];
    printf("Номер [%s]: ", c->phonenumber[index].number);
    if(fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(buffer[0] != '\0')
        {
            strcpy(c->phonenumber[index].number, buffer);
        }
    }
}
static void editSocial(Contact* c, int index)
{
    if (index < 0)
    {
        fprintf(stderr, "Отрицательный индекс соц сети. Изменение невозможно!\n");
        return;
    }

    if (index >= c->countSocial)
    {
        fprintf(stderr, "Выход за границы списка соц сетей\n");
        return;
    }

    char buffer[MAX_SIZE_LINK];
    printf("\nИзмениние Соц сети (пусто - оставить без изменений)\n");
    printf("Никнейм [%s]: ", c->social[index].username);
    if(fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(buffer[0] != '\0')
        {
            strncpy(c->social[index].username, buffer, sizeof(c->social[index].username) - 1);
            c->social[index].username[sizeof(c->social[index].username) - 1] = '\0';
        }
    }

    printf("Тип соц сети [%s]: ", c->social[index].typeSocial);
    if(fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(buffer[0] != '\0')
        {
            strncpy(c->social[index].typeSocial, buffer, sizeof(c->social[index].typeSocial) - 1);
            c->social[index].typeSocial[sizeof(c->social[index].typeSocial) - 1] = '\0';
        }
    }

    printf("Ссылка на профиль [%s]: ", c->social[index].link);
    if(fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(buffer[0] != '\0')
        {
            strcpy(c->social[index].link, buffer);
        }
    }
}


static void printContact(const Contact* c)
{
    printf("Информация о контакте: \nФамилия: %s\nИмя: %s\nОтчество: %s\n", c->surname, c->name, c->patronymic);
    printf("\n----------------------------\n");
    printf("Социальные сети: ");
    if (c->countSocial == 0)
    {
        printf("-\n");
    }
    else
    {

        for (int i = 0; i < c->countSocial; ++i)
        {
            printf("\nСоц сеть #%d\n", i + 1);
            printf("Тип соцсети: %s\nСсылка на профиль: %s\nНикнейм: %s\n",
                c->social[i].typeSocial,
                c->social[i].link,
                c->social[i].username);
        }
    }
    printf("\n----------------------------\n");
    printf("Номера телефонов: ");
    if(c->countNumbers == 0)
    {
        printf("-\n");
    }
    else
    {
        for (int i = 0; i < c->countNumbers; ++i)
        {
            printf("\n");
            printf("Номер #%d  %s", i + 1, c->phonenumber[i].number);
        }
        printf("\n----------------------------\n");
    }
}
static void printPhonebook(const PhonebookList* pb)
{
    if (pb == NULL || pb->head == NULL)
    {
        printf("\nТелефонная книга пуста\n");
        return;
    }

    int i = 1;
    for (const PHNode* node = pb->head; node != NULL; node = node->next)
    {
        printf("\n-----------------------\n");
        printf("Контакт #%d\n", i);
        printContact(&node->data);
        i++;
    }
}

static void fillTestData(PhonebookList* pb)
{
    const char* surnames[MAX_COUNT_CONTACTS] = {
        "Иванов", "Петрова", "Сидоров", "Кузнецова",
        "Смирнов", "Волкова", "Морозов", "Соколова"
    };
    const char* names[MAX_COUNT_CONTACTS] = {
        "Иван", "Мария", "Алексей", "Ольга",
        "Дмитрий", "Анна", "Сергей", "Екатерина"
    };
    const char* patronymics[MAX_COUNT_CONTACTS] = {
        "Иванович", "Александровна", "Петрович", "Сергеевна",
        "Дмитриевич", "Викторовна", "Николаевич", "Андреевна"
    };
    const char* socialTypes[MAX_COUNT] = { "Telegram", "VK", "Instagram", "WhatsApp" };

    for (int i = 0; i < MAX_COUNT_CONTACTS; ++i)
    {
        Contact c;
        memset(&c, 0, sizeof(c));

        strcpy(c.surname, surnames[i]);
        strcpy(c.name, names[i]);
        strcpy(c.patronymic, patronymics[i]);

        c.countNumbers = MAX_COUNT;
        for (int j = 0; j < MAX_COUNT; ++j)
        {
            snprintf(c.phonenumber[j].number, sizeof(c.phonenumber[j].number),
                     "+7-9%02d-%03d-%02d-%02d", i * 10 + j, j * 111, j * 11, j * 7);
        }

        c.countSocial = MAX_COUNT;
        for (int j = 0; j < MAX_COUNT; ++j)
        {
            strcpy(c.social[j].typeSocial, socialTypes[j]);
            snprintf(c.social[j].username, sizeof(c.social[j].username),
                     "%s_%s_%d", names[i], surnames[i], j + 1);
            snprintf(c.social[j].link, sizeof(c.social[j].link),
                     "https://example.com/profile/%d/%d", i + 1, j + 1);
        }

        phlist_insert(pb, &c);
    }
}


void print_interface(void)
{
    PhonebookList* pb = phlist_init();
    if (pb == NULL) return;

    fillTestData(pb);

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
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        line[strcspn(line, "\n")] = '\0';
        int choice = atoi(line);

        switch((MenuAction)choice)
        {
            case MENU_EXIT:
                running = 0;
                break;
            case MENU_ADD:
            {
                Contact c;
                if (addContact(&c))
                {
                    phlist_insert(pb, &c);
                }
                break;
            }
            case MENU_DELETE:
            {
                printf("\n-------------------\n");
                printf("Какой контакт хотите удалить?: ");

                char line2[8];
                if (fgets(line2, sizeof(line2), stdin) == NULL) break;
                line2[strcspn(line2, "\n")] = '\0';
                int delChoice = atoi(line2);

                PHNode* node = (delChoice > 0)
                             ? phlist_at(pb, (size_t)(delChoice - 1))
                             : NULL;

                if (node == NULL)
                {
                    fprintf(stderr, "Некорректный номер контакта. Попробуйте снова\n");
                    break;
                }

                phlist_erase(pb, node);
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
                if (fgets(lineC, sizeof(lineC), stdin) == NULL) break;
                lineC[strcspn(lineC, "\n")] = '\0';
                int contactNum = atoi(lineC);

                PHNode* node = (contactNum > 0)
                             ? phlist_at(pb, (size_t)(contactNum - 1))
                             : NULL;

                if (node == NULL)
                {
                    fprintf(stderr, "Некорректный номер контакта. Попробуйте снова\n");
                    break;
                }

                Contact* c = &node->data;
                printf("\nСписок доступных изменений:\n");
                printf("%d. Изменить Ф.И.О\n", EDIT_NAMES);
                printf("%d. Изменить соц сети\n", EDIT_SOCIAL);
                printf("%d. Удалить соц сети\n", EDIT_SOCIAL_DELETE);
                printf("%d. Изменить номер телефона\n", EDIT_PHONE);
                printf("%d. Удалить номер телефона\n", EDIT_PHONE_DELETE);

                printf("Выберите действие ");
                char line3[8];
                if (fgets(line3, sizeof(line3), stdin) == NULL) break;
                line3[strcspn(line3, "\n")] = '\0';
                int choice3 = atoi(line3);

                switch((EditAction)choice3)
                {
                    case EDIT_EXIT: break;
                    case EDIT_NAMES:
                    {
                        editNames(c);
                        phlist_reposition(pb, node);
                        break;
                    }
                    case EDIT_SOCIAL:
                    {
                        printf("Выберите номер соц сети: ");

                        char line3_2[8];
                        if (fgets(line3_2, sizeof(line3_2), stdin) == NULL) break;
                        line3_2[strcspn(line3_2, "\n")] = '\0';
                        int socialNum = atoi(line3_2);

                        if(socialNum < 1 || socialNum > c->countSocial)
                        {
                            fprintf(stderr, "Некорректный номер соц сети\n");
                            break;
                        }
                        else
                        {
                            editSocial(c, socialNum - 1);
                        }
                        break;
                    }
                    case EDIT_SOCIAL_DELETE:
                    {
                        printf("Выберите номер соц сети: ");

                        char line3_3[8];
                        if (fgets(line3_3, sizeof(line3_3), stdin) == NULL) break;
                        line3_3[strcspn(line3_3, "\n")] = '\0';
                        int socialNum = atoi(line3_3);

                        if(socialNum < 1 || socialNum > c->countSocial)
                        {
                            fprintf(stderr, "Некорректный номер соц сети\n");
                            break;
                        }
                        else
                        {
                            deleteSocial(c, socialNum - 1);
                        }
                        break;
                    }
                    case EDIT_PHONE:
                    {
                        printf("\nИзменение номера телефона (пусто - оставить без изменений)\n");

                        printf("Выберите номер номера: ");
                        char line3_4[8];
                        if (fgets(line3_4, sizeof(line3_4), stdin) == NULL) break;
                        line3_4[strcspn(line3_4, "\n")] = '\0';
                        int numNum = atoi(line3_4);

                        if(numNum < 1 || numNum > c->countNumbers)
                        {
                            fprintf(stderr, "Некорректный номер номера\n");
                            break;
                        }
                        else
                        {
                            editNumber(c, numNum - 1);
                        }
                        break;
                    }
                    case EDIT_PHONE_DELETE:
                    {
                        printf("Выберите номер номера (ага номер номера): ");

                        char line3_5[8];
                        if (fgets(line3_5, sizeof(line3_5), stdin) == NULL) break;
                        line3_5[strcspn(line3_5, "\n")] = '\0';
                        int numNum = atoi(line3_5);

                        if(numNum < 1 || numNum > c->countNumbers)
                        {
                            fprintf(stderr, "Некорректный номер номера\n");
                            break;
                        }
                        else
                        {
                            deleteNumber(c, numNum - 1);
                        }
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

    phlist_free(&pb);
}
