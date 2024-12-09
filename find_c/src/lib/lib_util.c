#include "lib/lib_util.h"

int my_stroi(char *str, size_t i)
{
    int c = ((str[i] - 48) % 10) * 100;
    int d = ((str[1 + i] - 48) % 10) * 10;
    int u = ((str[2 + i] - 48) % 10);
    return c + d + u;
}

int octal_to_dec(int n)
{
    int c = n / 100;
    int d = (n - c * 100) / 10;
    int u = (n - c * 100 - d * 10);
    return c * 8 * 8 + d * 8 + u;
}