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

static void printPhonebook(const PhonebookTree* pb);
static void printContact(const Contact* c);
static void printTreeShape(const PHNode* node, int depth);
static void printTreeInfo(const PhonebookTree* pb);

static void phtree_free_nodes(PHNode* node);
static PHNode* phtree_min(PHNode* node);
static void phtree_replace(PhonebookTree* phtree, PHNode* node, PHNode* child);
static size_t node_height(const PHNode* node);
static void collect_sorted(PHNode* node, PHNode** array, size_t* pos);
static PHNode* build_balanced(PHNode** array, size_t lo, size_t hi, PHNode* parent);
static void phtree_maybe_balance(PhonebookTree* phtree);


int contact_cmp(const Contact* a, const Contact* b)
{
    int result = strcmp(a->surname, b->surname);
    if (result != 0) return result;

    result = strcmp(a->name, b->name);
    if (result != 0) return result;

    return strcmp(a->patronymic, b->patronymic);
}

PhonebookTree* phtree_init(void)
{
    PhonebookTree* phtree = malloc(sizeof(PhonebookTree));
    if (phtree == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для телефонной книги\n");
        return NULL;
    }

    phtree->root = NULL;
    phtree->size = 0;
    phtree->opsSinceBalance = 0;

    return phtree;
}

static void phtree_free_nodes(PHNode* node)
{
    if (node == NULL) return;

    phtree_free_nodes(node->left);
    phtree_free_nodes(node->right);

    free(node);
}

void phtree_free(PhonebookTree** phtree)
{
    if (phtree == NULL || *phtree == NULL) return;

    phtree_free_nodes((*phtree)->root);

    free(*phtree);
    *phtree = NULL;
}

static size_t node_height(const PHNode* node)
{
    if (node == NULL) return 0;

    size_t left = node_height(node->left);
    size_t right = node_height(node->right);

    return (left > right ? left : right) + 1;
}

size_t phtree_height(const PhonebookTree* phtree)
{
    if (phtree == NULL) return 0;

    return node_height(phtree->root);
}

/* Высота идеально сбалансированного дерева из size узлов: floor(log2(size)) + 1 */
size_t phtree_ideal_height(size_t size)
{
    size_t height = 0;

    while (size > 0)
    {
        size >>= 1;
        height++;
    }

    return height;
}

/* Обход по возрастанию - узлы попадают в массив уже упорядоченными по Ф.И.О. */
static void collect_sorted(PHNode* node, PHNode** array, size_t* pos)
{
    if (node == NULL) return;

    collect_sorted(node->left, array, pos);
    array[(*pos)++] = node;
    collect_sorted(node->right, array, pos);
}

/* Середина отрезка становится корнем поддерева, половины - его детьми.
   hi - индекс за последним элементом. Память не перевыделяется,
   переписываются только связи между уже существующими узлами. */
static PHNode* build_balanced(PHNode** array, size_t lo, size_t hi, PHNode* parent)
{
    if (lo >= hi) return NULL;

    size_t mid = lo + (hi - lo) / 2;
    PHNode* node = array[mid];

    node->parent = parent;
    node->left = build_balanced(array, lo, mid, node);
    node->right = build_balanced(array, mid + 1, hi, node);

    return node;
}

int phtree_balance(PhonebookTree* phtree)
{
    if (phtree == NULL) return 0;

    phtree->opsSinceBalance = 0;

    if (phtree->size < 3) return 0;

    PHNode** array = malloc(phtree->size * sizeof(PHNode*));
    if (array == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для балансировки дерева\n");
        return 0;
    }

    size_t pos = 0;
    collect_sorted(phtree->root, array, &pos);

    phtree->root = build_balanced(array, 0, pos, NULL);

    free(array);
    return 1;
}

/* Периодическая проверка: раз в BALANCE_PERIOD изменений смотрим высоту
   и перестраиваем дерево, только если оно реально выродилось */
static void phtree_maybe_balance(PhonebookTree* phtree)
{
    phtree->opsSinceBalance++;
    if (phtree->opsSinceBalance < BALANCE_PERIOD) return;

    size_t height = node_height(phtree->root);
    size_t limit = phtree_ideal_height(phtree->size) * BALANCE_FACTOR;

    if (height <= limit)
    {
        phtree->opsSinceBalance = 0;
        return;
    }

    if (phtree_balance(phtree))
    {
        printf("\n[Дерево перестроено: высота %lu -> %lu при %lu контактах]\n",
               (unsigned long)height,
               (unsigned long)node_height(phtree->root),
               (unsigned long)phtree->size);
    }
}

PHNode* phtree_insert(PhonebookTree* phtree, const Contact* c)
{
    if (phtree == NULL || c == NULL) return NULL;

    PHNode* node = malloc(sizeof(PHNode));
    if (node == NULL)
    {
        fprintf(stderr, "Ошибка выделения памяти для контакта\n");
        return NULL;
    }

    node->data = *c;
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;

    PHNode* parent = NULL;
    PHNode* pos = phtree->root;

    while (pos != NULL)
    {
        parent = pos;
        /* однофамильцы с одинаковым Ф.И.О. уходят вправо - порядок добавления сохраняется */
        pos = (contact_cmp(c, &pos->data) < 0) ? pos->left : pos->right;
    }

    node->parent = parent;

    if (parent == NULL)
    {
        phtree->root = node;
    }
    else if (contact_cmp(c, &parent->data) < 0)
    {
        parent->left = node;
    }
    else
    {
        parent->right = node;
    }

    phtree->size++;
    phtree_maybe_balance(phtree);

    return node;
}

static PHNode* phtree_min(PHNode* node)
{
    while (node->left != NULL)
    {
        node = node->left;
    }

    return node;
}

/* Подставить child на место node в связях с родителем */
static void phtree_replace(PhonebookTree* phtree, PHNode* node, PHNode* child)
{
    if (node->parent == NULL)
    {
        phtree->root = child;
    }
    else if (node->parent->left == node)
    {
        node->parent->left = child;
    }
    else
    {
        node->parent->right = child;
    }

    if (child != NULL)
    {
        child->parent = node->parent;
    }
}

void phtree_erase(PhonebookTree* phtree, PHNode* node)
{
    if (phtree == NULL || node == NULL) return;

    if (node->left != NULL && node->right != NULL)
    {
        /* Два ребёнка: переносим в узел данные преемника (самого левого справа),
           а удаляем уже сам преемник - у него левого ребёнка заведомо нет */
        PHNode* successor = phtree_min(node->right);

        node->data = successor->data;
        node = successor;
    }

    PHNode* child = (node->left != NULL) ? node->left : node->right;

    phtree_replace(phtree, node, child);

    free(node);
    phtree->size--;

    phtree_maybe_balance(phtree);
}

/* Ф.И.О. - ключ дерева, после его правки узел обязан встать на новое место.
   Данные копируются до удаления: phtree_erase может освободить другой узел. */
PHNode* phtree_reposition(PhonebookTree* phtree, PHNode* node)
{
    if (phtree == NULL || node == NULL) return NULL;

    Contact copy = node->data;

    phtree_erase(phtree, node);

    return phtree_insert(phtree, &copy);
}

static PHNode* at_helper(PHNode* node, size_t* index)
{
    if (node == NULL) return NULL;

    PHNode* found = at_helper(node->left, index);
    if (found != NULL) return found;

    if (*index == 0) return node;
    (*index)--;

    return at_helper(node->right, index);
}

PHNode* phtree_at(const PhonebookTree* phtree, size_t index)
{
    if (phtree == NULL || index >= phtree->size) return NULL;

    return at_helper(phtree->root, &index);
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

static void printSubtree(const PHNode* node, int* number)
{
    if (node == NULL) return;

    printSubtree(node->left, number);

    printf("\n-----------------------\n");
    printf("Контакт #%d\n", *number);
    printContact(&node->data);
    (*number)++;

    printSubtree(node->right, number);
}

static void printPhonebook(const PhonebookTree* pb)
{
    if (pb == NULL || pb->root == NULL)
    {
        printf("\nТелефонная книга пуста\n");
        return;
    }

    int number = 1;
    printSubtree(pb->root, &number);
}

/* Дерево, повёрнутое на 90 градусов: правое поддерево сверху, левое снизу */
static void printTreeShape(const PHNode* node, int depth)
{
    if (node == NULL) return;

    printTreeShape(node->right, depth + 1);
    printf("%*s%s %s\n", depth * 4, "", node->data.surname, node->data.name);
    printTreeShape(node->left, depth + 1);
}

static void printTreeInfo(const PhonebookTree* pb)
{
    if (pb == NULL || pb->root == NULL)
    {
        printf("\nТелефонная книга пуста\n");
        return;
    }

    printf("\n-----------------------\n");
    printf("Контактов: %lu\n", (unsigned long)pb->size);
    printf("Высота дерева: %lu (идеальная %lu, порог %lu)\n",
           (unsigned long)phtree_height(pb),
           (unsigned long)phtree_ideal_height(pb->size),
           (unsigned long)(phtree_ideal_height(pb->size) * BALANCE_FACTOR));
    printf("Изменений с последней проверки: %lu из %d\n",
           (unsigned long)pb->opsSinceBalance, BALANCE_PERIOD);
    printf("-----------------------\n");

    printTreeShape(pb->root, 0);
}

static void fillTestData(PhonebookTree* pb)
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

        phtree_insert(pb, &c);
    }
}


void print_interface(void)
{
    PhonebookTree* pb = phtree_init();
    if (pb == NULL) return;

    fillTestData(pb);

    int running = 1;
    while (running)
    {
        printf("\n%d. Добавить контакт\n", MENU_ADD);
        printf("%d. Удалить контакт\n", MENU_DELETE);
        printf("%d. Изменить контакт\n", MENU_EDIT);
        printf("%d. Вывести все контакты\n", MENU_PRINT);
        printf("%d. Показать структуру дерева\n", MENU_TREE);
        printf("%d. Сбалансировать дерево\n", MENU_BALANCE);
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
                    phtree_insert(pb, &c);
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
                             ? phtree_at(pb, (size_t)(delChoice - 1))
                             : NULL;

                if (node == NULL)
                {
                    fprintf(stderr, "Некорректный номер контакта. Попробуйте снова\n");
                    break;
                }

                phtree_erase(pb, node);
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
                             ? phtree_at(pb, (size_t)(contactNum - 1))
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
                        /* после этого вызова node и c недействительны */
                        phtree_reposition(pb, node);
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
            case MENU_TREE:
            {
                printTreeInfo(pb);
                break;
            }
            case MENU_BALANCE:
            {
                size_t before = phtree_height(pb);

                if (!phtree_balance(pb))
                {
                    printf("\nВ дереве меньше трёх контактов, перестраивать нечего\n");
                    break;
                }

                printf("\nВысота дерева: %lu -> %lu\n",
                       (unsigned long)before, (unsigned long)phtree_height(pb));
                break;
            }
            default:
            {
                fprintf(stderr, "Неизвестное дейтсвие\n");
                break;
            }
        }
    }

    phtree_free(&pb);
}
