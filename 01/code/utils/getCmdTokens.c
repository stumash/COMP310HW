#include <utils/getCmdTokens.h>

void
getCmdTokens(char **args, int *tok_len)
{
    while (1)
    {
        //-----------------------------------
        // 1: get a non-empty line of input
        //-----------------------------------
        fprintf(stdout, "sh> ");

        char buffer[BUFSIZE];
        int linelen = getLine(buffer, BUFSIZE, stdin);
        if (linelen == -1)
        {
            printf("error: length of input string ≥ %d\n", BUFSIZE);
            continue;
        }
        else if (linelen == 0)
        {
            continue;
        }

        //---------------------------------------
        // 2: parse input into array of strings
        //---------------------------------------
        int tokMaxLen = maxTokenLen(buffer, linelen);
        *tok_len = tokMaxLen;

        int tokMinLen = minTokenLen(buffer, linelen);
        int maxTokQty = linelen / tokMinLen;

        for (int i = 0; i < maxTokQty; i++)
        {
            // TODO: allocate string of len tokMaxLen and
            // parse next token into it
        }
    }
}
