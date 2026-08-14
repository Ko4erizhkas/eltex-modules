#include <stdio.h>
#include <stdlib.h>

#include "ipnet.h"

int ip_parse(const char* str, unsigned* addr)
{
    unsigned octet[4];
    char tail;
    int i;

    if (sscanf(str, "%u.%u.%u.%u%c", &octet[0], &octet[1], &octet[2], &octet[3], &tail) != 4)
        return 0;

    for (i = 0; i < 4; i++)
    {
        if (octet[i] > 255)
            return 0;
    }

    *addr = (octet[0] << 24) | (octet[1] << 16) | (octet[2] << 8) | octet[3];
    return 1;
}

void ip_format(unsigned addr, char* out)
{
    sprintf(out, "%u.%u.%u.%u",
            (addr >> 24) & 0xFF,
            (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF,
            addr & 0xFF);
}

int mask_is_valid(unsigned mask)
{
    unsigned inv = ~mask;
    return (inv & (inv + 1)) == 0;
}

int ip_same_subnet(unsigned a, unsigned b, unsigned mask)
{
    return (a & mask) == (b & mask);
}

unsigned ip_random(void)
{
    unsigned addr = 0;
    int i;

    for (i = 0; i < 4; i++)
        addr = (addr << 8) | (unsigned)(rand() & 0xFF);

    return addr;
}
