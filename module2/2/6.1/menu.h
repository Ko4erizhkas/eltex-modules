#ifndef MENU_H
#define MENU_H

/* Пользовательская часть программы: ввод, редактирование, печать, меню.
   Со списком работает только через интерфейс библиотеки phoneBook.h */

#define MAX_COUNT_CONTACTS 8

typedef enum
{
    MENU_EXIT = 0,
    MENU_ADD = 1,
    MENU_DELETE = 2,
    MENU_EDIT = 3,
    MENU_PRINT = 4
} MenuAction;

typedef enum
{
    EDIT_EXIT = 0,
    EDIT_NAMES = 1,
    EDIT_SOCIAL = 2,
    EDIT_SOCIAL_DELETE = 3,
    EDIT_PHONE = 4,
    EDIT_PHONE_DELETE = 5
} EditAction;

void print_interface(void);

#endif
