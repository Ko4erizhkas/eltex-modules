#ifndef PERM_H
#define PERM_H

#define MAX_INPUT 512

/* Маска значащих битов: setuid/setgid/sticky + rwx для владельца/группы/остальных */
#define PERM_MASK 07777

#define PERM_SUID 04000
#define PERM_SGID 02000
#define PERM_SVTX 01000

/* Классы пользователей для chmod-выражений */
#define WHO_U 1
#define WHO_G 2
#define WHO_O 4
#define WHO_A (WHO_U | WHO_G | WHO_O)

typedef enum
{
    MENU_EXIT = 0,
    MENU_INPUT = 1,
    MENU_FILE = 2,
    MENU_CHMOD = 3,
    MENU_SHOW = 4
} MenuAction;

typedef struct Perm
{
    unsigned mode; /* младшие 12 бит прав доступа */
    char type;     /* символ типа файла в стиле ls -l: '-', 'd', 'l', ... */
    int isSet;     /* 0 - права ещё не заданы */

} Perm;

int perm_parse_octal(const char* str, unsigned* mode);
int perm_parse_symbolic(const char* str, unsigned* mode);
int perm_parse_any(const char* str, unsigned* mode);

int perm_from_file(const char* path, Perm* perm);
int perm_chmod(const char* expr, Perm* perm);

void perm_to_octal(unsigned mode, char* out);    /* out - минимум 5 байт  */
void perm_to_symbolic(unsigned mode, char* out); /* out - минимум 10 байт */
void perm_to_binary(unsigned mode, char* out);   /* out - минимум 16 байт */
void perm_print(const Perm* perm);

void print_interface(void);

#endif
