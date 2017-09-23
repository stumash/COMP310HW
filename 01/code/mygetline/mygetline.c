#include <stdio.h>
#include <sys/types.h>

#include "mygetline.h"

size_t mygetline(char *buffer, size_t n, FILE *stream)
{
    char c;
    int i = 0;
    while (1)
    {
        c = getc(stream);

        if (c == '\n' || c == EOF || i == (int)n-1)
        {
            buffer[i] = '\0';
            return i;
        }
        else
        {
            buffer[i] = c;
            i++;
        }
    }
}
