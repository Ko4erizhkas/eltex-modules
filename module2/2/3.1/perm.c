#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "perm.h"

/* ls -l показывает сам симлинк, а не то, на что он указывает */
#ifdef _WIN32
#define stat_file stat
#else
#define stat_file lstat
#endif

static const unsigned readBit[3] = { 0400, 0040, 0004 };
static const unsigned writeBit[3] = { 0200, 0020, 0002 };
static const unsigned execBit[3] = { 0100, 0010, 0001 };
static const unsigned specialBit[3] = { PERM_SUID, PERM_SGID, PERM_SVTX };
static const char specialLetter[3] = { 's', 's', 't' };

static char exec_char(unsigned mode, int index);
static char file_type_char(unsigned stMode);

static unsigned spread(int who, unsigned rwx);
static unsigned clear_mask(int who);
static unsigned class_rwx(unsigned mode, char who);

static int parse_perm_list(const char** pp, int who, unsigned mode, int isDir, unsigned* bits);
static int apply_clause(const char** pp, unsigned* mode, int isDir);

static int read_line(char* buf, size_t size);


static char exec_char(unsigned mode, int index)
{
    int hasExec = (mode & execBit[index]) != 0;

    if (mode & specialBit[index])
    {
        char letter = specialLetter[index];
        return hasExec ? letter : (char)toupper((unsigned char)letter);
    }

    return hasExec ? 'x' : '-';
}

void perm_to_symbolic(unsigned mode, char* out)
{
    for (int i = 0; i < 3; i++)
    {
        out[i * 3] = (mode & readBit[i]) ? 'r' : '-';
        out[i * 3 + 1] = (mode & writeBit[i]) ? 'w' : '-';
        out[i * 3 + 2] = exec_char(mode, i);
    }

    out[9] = '\0';
}

void perm_to_octal(unsigned mode, char* out)
{
    sprintf(out, "%04o", mode & PERM_MASK);
}

void perm_to_binary(unsigned mode, char* out)
{
    int pos = 0;

    for (int bit = 11; bit >= 0; bit--)
    {
        out[pos++] = (mode & (1u << bit)) ? '1' : '0';

        if (bit % 3 == 0 && bit != 0)
        {
            out[pos++] = ' ';
        }
    }

    out[pos] = '\0';
}

int perm_parse_octal(const char* str, unsigned* mode)
{
    if (str == NULL || str[0] == '\0') return 0;

    size_t len = strlen(str);
    if (len > 4) return 0;

    unsigned value = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (str[i] < '0' || str[i] > '7') return 0;
        value = value * 8 + (unsigned)(str[i] - '0');
    }

    *mode = value & PERM_MASK;
    return 1;
}

int perm_parse_symbolic(const char* str, unsigned* mode)
{
    if (str == NULL) return 0;

    size_t len = strlen(str);

    /* строку можно скопировать прямо из ls -l - вместе с символом типа файла */
    if (len == 10) str++;
    else if (len != 9) return 0;

    unsigned value = 0;
    for (int i = 0; i < 3; i++)
    {
        char r = str[i * 3];
        char w = str[i * 3 + 1];
        char x = str[i * 3 + 2];

        if (r == 'r') value |= readBit[i];
        else if (r != '-') return 0;

        if (w == 'w') value |= writeBit[i];
        else if (w != '-') return 0;

        char letter = specialLetter[i];
        char upper = (char)toupper((unsigned char)letter);

        if (x == 'x') value |= execBit[i];
        else if (x == letter) value |= execBit[i] | specialBit[i];
        else if (x == upper) value |= specialBit[i];
        else if (x != '-') return 0;
    }

    *mode = value;
    return 1;
}

int perm_parse_any(const char* str, unsigned* mode)
{
    if (str == NULL || str[0] == '\0') return 0;

    if (isdigit((unsigned char)str[0]))
    {
        return perm_parse_octal(str, mode);
    }

    return perm_parse_symbolic(str, mode);
}

static char file_type_char(unsigned stMode)
{
    if (S_ISDIR(stMode)) return 'd';
    if (S_ISCHR(stMode)) return 'c';
#ifdef S_ISBLK
    if (S_ISBLK(stMode)) return 'b';
#endif
#ifdef S_ISFIFO
    if (S_ISFIFO(stMode)) return 'p';
#endif
#ifdef S_ISLNK
    if (S_ISLNK(stMode)) return 'l';
#endif
#ifdef S_ISSOCK
    if (S_ISSOCK(stMode)) return 's';
#endif

    return '-';
}

int perm_from_file(const char* path, Perm* perm)
{
    if (path == NULL || path[0] == '\0' || perm == NULL) return 0;

    struct stat info;
    if (stat_file(path, &info) != 0)
    {
        perror("Не удалось получить информацию о файле");
        return 0;
    }

    perm->mode = (unsigned)info.st_mode & PERM_MASK;
    perm->type = file_type_char((unsigned)info.st_mode);
    perm->isSet = 1;

    return 1;
}

static unsigned spread(int who, unsigned rwx)
{
    unsigned bits = 0;

    if (who & WHO_U) bits |= rwx << 6;
    if (who & WHO_G) bits |= rwx << 3;
    if (who & WHO_O) bits |= rwx;

    return bits;
}

static unsigned clear_mask(int who)
{
    unsigned mask = spread(who, 7);

    if (who & WHO_U) mask |= PERM_SUID;
    if (who & WHO_G) mask |= PERM_SGID;
    if (who & WHO_O) mask |= PERM_SVTX;

    return mask;
}

static unsigned class_rwx(unsigned mode, char who)
{
    switch (who)
    {
        case 'u': return (mode >> 6) & 7;
        case 'g': return (mode >> 3) & 7;
        default:  return mode & 7;
    }
}

/* Разбор правой части клаузы: rwxXst либо одна буква u/g/o (копия класса) */
static int parse_perm_list(const char** pp, int who, unsigned mode, int isDir, unsigned* bits)
{
    const char* p = *pp;

    unsigned rwx = 0;
    unsigned special = 0;

    if (*p == 'u' || *p == 'g' || *p == 'o')
    {
        rwx = class_rwx(mode, *p);
        p++;
    }
    else
    {
        while (*p != '\0' && strchr("rwxXst", *p) != NULL)
        {
            switch (*p)
            {
                case 'r': rwx |= 4; break;
                case 'w': rwx |= 2; break;
                case 'x': rwx |= 1; break;
                /* X - только для каталога или если бит x уже где-то стоит */
                case 'X': if (isDir || (mode & 0111)) rwx |= 1; break;
                case 's':
                    if (who & WHO_U) special |= PERM_SUID;
                    if (who & WHO_G) special |= PERM_SGID;
                    break;
                default: special |= PERM_SVTX; break; /* 't' */
            }

            p++;
        }
    }

    *bits = spread(who, rwx) | special;
    *pp = p;

    return 1;
}

/* Одна клауза вида [ugoa...][+-=][права...], например ug+rw-x */
static int apply_clause(const char** pp, unsigned* mode, int isDir)
{
    const char* p = *pp;

    int who = 0;
    while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a')
    {
        switch (*p)
        {
            case 'u': who |= WHO_U; break;
            case 'g': who |= WHO_G; break;
            case 'o': who |= WHO_O; break;
            default:  who |= WHO_A; break;
        }

        p++;
    }

    if (who == 0) who = WHO_A;

    if (*p != '+' && *p != '-' && *p != '=') return 0;

    while (*p == '+' || *p == '-' || *p == '=')
    {
        char op = *p++;

        unsigned bits = 0;
        if (!parse_perm_list(&p, who, *mode, isDir, &bits)) return 0;

        switch (op)
        {
            case '+': *mode |= bits; break;
            case '-': *mode &= ~bits; break;
            default:  *mode = (*mode & ~clear_mask(who)) | bits; break;
        }
    }

    *pp = p;
    return 1;
}

int perm_chmod(const char* expr, Perm* perm)
{
    if (expr == NULL || expr[0] == '\0' || perm == NULL) return 0;

    if (isdigit((unsigned char)expr[0]))
    {
        unsigned mode;
        if (!perm_parse_octal(expr, &mode)) return 0;

        perm->mode = mode;
        return 1;
    }

    unsigned mode = perm->mode;
    int isDir = (perm->type == 'd');

    const char* p = expr;
    while (*p != '\0')
    {
        if (!apply_clause(&p, &mode, isDir)) return 0;

        if (*p == ',')
        {
            p++;
            if (*p == '\0') return 0;
        }
        else if (*p != '\0')
        {
            return 0;
        }
    }

    perm->mode = mode;
    return 1;
}

void perm_print(const Perm* perm)
{
    char symbolic[10];
    char octal[5];
    char binary[16];

    perm_to_symbolic(perm->mode, symbolic);
    perm_to_octal(perm->mode, octal);
    perm_to_binary(perm->mode, binary);

    printf("\n");
    printf("Буквенно: %c%s\n", perm->type, symbolic);
    printf("Цифрами:  %s\n", octal);
    printf("Битами:   %s\n", binary);
    printf("          спец u   g   o\n");
}

static int read_line(char* buf, size_t size)
{
    if (fgets(buf, size, stdin) == NULL) return 0;

    buf[strcspn(buf, "\n")] = '\0';
    return 1;
}

void print_interface(void)
{
    Perm perm = { 0, '-', 0 };

    int running = 1;
    while (running)
    {
        printf("\n%d. Ввести права буквами или цифрами\n", MENU_INPUT);
        printf("%d. Взять права у файла\n", MENU_FILE);
        printf("%d. Изменить текущие права (как chmod)\n", MENU_CHMOD);
        printf("%d. Показать текущие права\n", MENU_SHOW);
        printf("%d. Выйти\n", MENU_EXIT);

        printf("Выберите действие: ");
        char line[MAX_INPUT];
        if (!read_line(line, sizeof(line))) break;

        if (line[0] == '\0' || line[1] != '\0' || !isdigit((unsigned char)line[0]))
        {
            fprintf(stderr, "Некорректный пункт меню. Попробуйте снова\n");
            continue;
        }

        switch ((MenuAction)(line[0] - '0'))
        {
            case MENU_EXIT:
                running = 0;
                break;

            case MENU_INPUT:
            {
                printf("\nВведите права (644, 0755, rw-r--r--, rwxr-sr-x): ");

                char input[MAX_INPUT];
                if (!read_line(input, sizeof(input)))
                {
                    running = 0;
                    break;
                }

                unsigned mode;
                if (!perm_parse_any(input, &mode))
                {
                    fprintf(stderr, "Некорректная запись прав. Попробуйте снова\n");
                    break;
                }

                perm.mode = mode;
                perm.type = '-';
                perm.isSet = 1;

                perm_print(&perm);
                break;
            }

            case MENU_FILE:
            {
                printf("\nВведите имя файла: ");

                char input[MAX_INPUT];
                if (!read_line(input, sizeof(input)))
                {
                    running = 0;
                    break;
                }

                if (!perm_from_file(input, &perm)) break;

                perm_print(&perm);
                break;
            }

            case MENU_CHMOD:
            {
                if (!perm.isSet)
                {
                    fprintf(stderr, "Права ещё не заданы. Сначала пункт %d или %d\n",
                            MENU_INPUT, MENU_FILE);
                    break;
                }

                printf("\nТекущие права:");
                perm_print(&perm);

                printf("\nВведите изменение (u+x, go-w, a=rw, o+t, 644): ");

                char input[MAX_INPUT];
                if (!read_line(input, sizeof(input)))
                {
                    running = 0;
                    break;
                }

                if (!perm_chmod(input, &perm))
                {
                    fprintf(stderr, "Некорректная команда изменения. Попробуйте снова\n");
                    break;
                }

                printf("\nНовые права:");
                perm_print(&perm);
                break;
            }

            case MENU_SHOW:
            {
                if (!perm.isSet)
                {
                    fprintf(stderr, "Права ещё не заданы. Сначала пункт %d или %d\n",
                            MENU_INPUT, MENU_FILE);
                    break;
                }

                perm_print(&perm);
                break;
            }

            default:
                fprintf(stderr, "Некорректный пункт меню. Попробуйте снова\n");
                break;
        }
    }
}
