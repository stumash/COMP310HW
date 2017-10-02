#include <utils/getCmdTokens.h>

void
getCmdTokens(char **args, int *n_tok, int *tok_len)
{
    while (1)
    {
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

        int tokMinLen = minTokenLen(buffer, linelen);
        int tokMaxLen = maxTokenLen(buffer, linelen);
        printf("min: %d, max: %d\n", (int)tokMinLen, (int)tokMaxLen);
    }
}
