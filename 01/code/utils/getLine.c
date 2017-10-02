#include "getLine.h"

int getLine(char *buffer, int n, FILE *stream)
{
    char c;
    int i = 0;
    while (1)
    {
        c = getc(stream);

        // if not at end of stream
        if (!charIsEndOfString(c))
        {
            // then if at end of buffer
            if (i == (int)n-1)
            {
                // null-terminate buffer and consume to end of stream
                buffer[i] = '\0';
                while (!charIsEndOfString(c)) { c = getc(stream); }
                return -1; // return error code
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

static int charIsEndOfString(char c)
{
    return (c == '\n') || (c == '\0') || (c == EOF);
}
