/*
 * Телефонная книга (список контактов).
 *
 * Хранит: Ф.И.О., место работы, должность, телефоны, email,
 * ссылки на соцсети, профили в мессенджерах.
 * Обязательны только фамилия и имя, остальное — по необходимости.
 * Данные хранятся в массивах. Доступны добавление, редактирование, удаление.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_CONTACTS 100   /* максимум контактов в книге              */
#define MAX_MULTI      5    /* максимум телефонов/email/ссылок и т.п.  */
#define STR_LEN      128    /* длина одного текстового поля            */

typedef struct {
    char surname[STR_LEN];      /* Фамилия   (обязательно) */
    char name[STR_LEN];         /* Имя       (обязательно) */
    char patronymic[STR_LEN];   /* Отчество                */
    char workplace[STR_LEN];    /* Место работы            */
    char position[STR_LEN];     /* Должность               */

    char phones[MAX_MULTI][STR_LEN];      int phone_count;
    char emails[MAX_MULTI][STR_LEN];      int email_count;
    char socials[MAX_MULTI][STR_LEN];     int social_count;
    char messengers[MAX_MULTI][STR_LEN];  int messenger_count;
} Contact;

static Contact contacts[MAX_CONTACTS];  /* массив контактов          */
static int contact_count = 0;           /* сколько сейчас заполнено   */

/* ---------------- Вспомогательный ввод ---------------- */

/* Прочитать строку целиком, убрать '\n', «хвост» сбросить из буфера. */
static void read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int c;                       /* строка не влезла — дочитываем */
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
}

/* Прочитать целое число (пункт меню). */
static int read_int(void)
{
    char buf[32];
    read_line(buf, sizeof buf);
    return atoi(buf);
}

/* Подтверждение да/нет. */
static int confirm(const char *prompt)
{
    char buf[16];
    printf("%s (y/n): ", prompt);
    read_line(buf, sizeof buf);
    return buf[0] == 'y' || buf[0] == 'Y';
}

/* Обязательное поле: не даём оставить пустым. */
static void read_required(const char *prompt, char *dest)
{
    do {
        printf("%s", prompt);
        read_line(dest, STR_LEN);
        if (dest[0] == '\0')
            printf("  Поле обязательно для заполнения.\n");
    } while (dest[0] == '\0');
}

/* Необязательное поле: пустая строка допустима. */
static void read_optional(const char *prompt, char *dest)
{
    printf("%s", prompt);
    read_line(dest, STR_LEN);
}

/* Ввод списка значений (телефоны, email и т.д.). */
static void read_multi(const char *label, char arr[][STR_LEN], int *count)
{
    *count = 0;
    printf("%s (пустая строка — завершить ввод, максимум %d):\n",
           label, MAX_MULTI);
    while (*count < MAX_MULTI) {
        char buf[STR_LEN];
        printf("  %d) ", *count + 1);
        read_line(buf, STR_LEN);
        if (buf[0] == '\0')
            break;
        strcpy(arr[*count], buf);
        (*count)++;
    }
}

/* ---------------- Вывод ---------------- */

static void print_multi(const char *label, const char arr[][STR_LEN], int count)
{
    if (count == 0)
        return;
    printf("  %s:\n", label);
    for (int i = 0; i < count; i++)
        printf("    - %s\n", arr[i]);
}

static void print_contact(int idx)
{
    const Contact *c = &contacts[idx];
    printf("[%d] %s %s", idx + 1, c->surname, c->name);
    if (c->patronymic[0])
        printf(" %s", c->patronymic);
    printf("\n");
    if (c->workplace[0])
        printf("  Место работы: %s\n", c->workplace);
    if (c->position[0])
        printf("  Должность: %s\n", c->position);
    print_multi("Телефоны", c->phones, c->phone_count);
    print_multi("Email", c->emails, c->email_count);
    print_multi("Соцсети", c->socials, c->social_count);
    print_multi("Мессенджеры", c->messengers, c->messenger_count);
}

static void list_contacts(void)
{
    if (contact_count == 0) {
        printf("Список контактов пуст.\n");
        return;
    }
    printf("\n===== Контакты (%d) =====\n", contact_count);
    for (int i = 0; i < contact_count; i++) {
        print_contact(i);
        printf("\n");
    }
}

/* Показать краткий список и выбрать контакт по номеру. Возврат -1 = отмена. */
static int select_contact(const char *action)
{
    if (contact_count == 0) {
        printf("Список контактов пуст.\n");
        return -1;
    }
    printf("\nКонтакты:\n");
    for (int i = 0; i < contact_count; i++) {
        const Contact *c = &contacts[i];
        printf("  %d) %s %s%s%s\n", i + 1, c->surname, c->name,
               c->patronymic[0] ? " " : "", c->patronymic);
    }
    printf("Выберите номер контакта для %s (0 — отмена): ", action);
    int n = read_int();
    if (n < 1 || n > contact_count) {
        if (n != 0)
            printf("Неверный номер.\n");
        return -1;
    }
    return n - 1;
}

/* ---------------- Операции ---------------- */

static void add_contact(void)
{
    if (contact_count >= MAX_CONTACTS) {
        printf("Книга переполнена (максимум %d контактов).\n", MAX_CONTACTS);
        return;
    }
    Contact *c = &contacts[contact_count];
    memset(c, 0, sizeof *c);

    printf("\n--- Новый контакт (поля со * обязательны) ---\n");
    read_required("Фамилия*: ", c->surname);
    read_required("Имя*: ", c->name);
    read_optional("Отчество: ", c->patronymic);
    read_optional("Место работы: ", c->workplace);
    read_optional("Должность: ", c->position);
    read_multi("Телефоны", c->phones, &c->phone_count);
    read_multi("Email", c->emails, &c->email_count);
    read_multi("Ссылки на соцсети", c->socials, &c->social_count);
    read_multi("Профили в мессенджерах", c->messengers, &c->messenger_count);

    contact_count++;
    printf("Контакт добавлен (всего: %d).\n", contact_count);
}

static void edit_contact(void)
{
    int idx = select_contact("редактирования");
    if (idx < 0)
        return;
    Contact *c = &contacts[idx];

    for (;;) {
        printf("\n--- Редактирование: %s %s ---\n", c->surname, c->name);
        printf("  1) Фамилия:      %s\n", c->surname);
        printf("  2) Имя:          %s\n", c->name);
        printf("  3) Отчество:     %s\n", c->patronymic);
        printf("  4) Место работы: %s\n", c->workplace);
        printf("  5) Должность:    %s\n", c->position);
        printf("  6) Телефоны      (%d)\n", c->phone_count);
        printf("  7) Email         (%d)\n", c->email_count);
        printf("  8) Соцсети       (%d)\n", c->social_count);
        printf("  9) Мессенджеры   (%d)\n", c->messenger_count);
        printf("  0) Завершить редактирование\n");
        printf("Что изменить? ");

        switch (read_int()) {
        case 1: read_required("Новая фамилия*: ", c->surname);            break;
        case 2: read_required("Новое имя*: ", c->name);                   break;
        case 3: read_optional("Отчество: ", c->patronymic);               break;
        case 4: read_optional("Место работы: ", c->workplace);            break;
        case 5: read_optional("Должность: ", c->position);                break;
        case 6: read_multi("Телефоны", c->phones, &c->phone_count);       break;
        case 7: read_multi("Email", c->emails, &c->email_count);          break;
        case 8: read_multi("Соцсети", c->socials, &c->social_count);      break;
        case 9: read_multi("Мессенджеры", c->messengers,
                           &c->messenger_count);                          break;
        case 0: printf("Изменения сохранены.\n");                         return;
        default: printf("Неверный пункт.\n");
        }
    }
}

static void delete_contact(void)
{
    int idx = select_contact("удаления");
    if (idx < 0)
        return;
    printf("Будет удалён: %s %s.\n", contacts[idx].surname, contacts[idx].name);
    if (!confirm("Удалить?")) {
        printf("Отменено.\n");
        return;
    }
    for (int i = idx; i < contact_count - 1; i++)  /* сдвиг массива влево */
        contacts[i] = contacts[i + 1];
    contact_count--;
    printf("Контакт удалён (осталось: %d).\n", contact_count);
}

/* ---------------- Главное меню ---------------- */

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    for (;;) {
        printf("\n===== Телефонная книга =====\n");
        printf("  1) Показать все контакты\n");
        printf("  2) Добавить контакт\n");
        printf("  3) Редактировать контакт\n");
        printf("  4) Удалить контакт\n");
        printf("  0) Выход\n");
        printf("Выбор: ");

        switch (read_int()) {
        case 1: list_contacts();   break;
        case 2: add_contact();     break;
        case 3: edit_contact();    break;
        case 4: delete_contact();  break;
        case 0: printf("Выход.\n"); return 0;
        default: printf("Неверный пункт меню.\n");
        }
    }
}