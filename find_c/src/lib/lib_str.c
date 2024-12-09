#include "lib/lib_str.h"

#include <stdlib.h>

size_t my_strlen(char *str)
{
    if (!str)
        return 0;
    size_t i = 0;
    while (str[i])
        i++;
    return i;
}

int my_strcmp(char *str1, char *str2)
{
    size_t s1 = my_strlen(str1);
    size_t s2 = my_strlen(str2);
    if (s1 != s2)
        return 1;
    for (size_t i = 0; i < s1; i++)
    {
        if (str1[i] != str2[i])
            return 1;
    }
    return 0;
}

char *my_strcp(char *str)
{
    size_t size = my_strlen(str);
    char *new_str = calloc(size + 1, 1);
    for (size_t i = 0; i < size; i++)
    {
        new_str[i] = str[i];
    }
    return new_str;
}

char *my_concate(char *dir, char *file)
{
    size_t ds = my_strlen(dir);
    size_t fs = my_strlen(file);
    char *new_str = calloc(ds + fs + 2, 1); // + 1 for \0 +1 for eventual '/'
    size_t i = 0;
    while (i < ds)
    {
        new_str[i] = dir[i];
        i++;
    }
    if (dir[ds - 1] != '/')
    {
        new_str[i] = '/';
        i++;
        ds++;
    }
    while (i - ds < fs)
    {
        new_str[i] = file[i - ds];
        i++;
    }
    return new_str;
}