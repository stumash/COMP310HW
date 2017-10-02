#include "maxTokenLen.h"

int maxTokenLen(char *s, int slen)
{
    int i = 0;
    int wlen = 0;
    int maxwlen = 0;
    while (i <= slen)
    {
        if (s[i] == ' ' || s[i] == '\t' || s[i] == '\0')
        {
            if (wlen)
            {
                if (wlen > maxwlen)
                {
                    maxwlen = wlen;
                }
            }

            wlen = 0;
        }
        else
        {
            wlen++;
        }

        i++;
    }

    return maxwlen;
}

