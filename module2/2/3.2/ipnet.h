#ifndef IPNET_H
#define IPNET_H

/* Разбор адреса вида "a.b.c.d" в 32-битное число. Возвращает 1 при успехе */
int ip_parse(const char* str, unsigned* addr);

/* Преобразование 32-битного числа в строку "a.b.c.d". out - минимум 16 байт */
void ip_format(unsigned addr, char* out);

/* Маска корректна, если это непрерывная последовательность единиц слева */
int mask_is_valid(unsigned mask);

/* Принадлежность адресов одной подсети */
int ip_same_subnet(unsigned a, unsigned b, unsigned mask);

/* Случайный IP адрес */
unsigned ip_random(void);

#endif
