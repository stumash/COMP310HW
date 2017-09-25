#include "getLine.h"

ssize_t getLine(char *buffer, size_t n, FILE *stream)
{
    char c;
    int i = 0;
    while (1)
    {
        c = getc(stream);

        // if not at end of string
        if (!(c == '\n' || c == EOF || c == '\0'))
        {
            // then if at end of buffer
            if (i == (int)n-1)
            {
                // end buffer and return error
                buffer[i] = '\0';
                return -1;
            }
            else
            {
                // else add char and keep going
                buffer[i] = c;
                i++;
            }
        }
        // if at end of string
        else
        {
            // end string and return its length
            buffer[i] = '\0';
            return i;
        }
    }
}
