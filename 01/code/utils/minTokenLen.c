#include "utils/minTokenLen.h"

int minTokenLen(char *s, int slen)
{
    int i = 0;
    int wlen = 0; // word length
    int minwlen = slen;
    while (i <= slen)
    {
        if (s[i] == ' ' || s[i] == '\t' || s[i] == '\0')
        {
            if (wlen) // if on space at end of word
            {
                if (wlen < minwlen)
                {
                    minwlen = wlen;
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

    return minwlen;
}
