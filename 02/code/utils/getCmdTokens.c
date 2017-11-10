#include <utils/getCmdTokens.h>

char **getCmdTokens(int *n_tokens, int *len)
{

    //-----------------------------------
    // 1: get a non-empty line of input
    //-----------------------------------
    char buffer[BUFSIZE];
    int linelen;
    while (1)
    {
        fprintf(stdout, ">> ");

        memset(buffer, 0, BUFSIZE);
        linelen = getLine(buffer, BUFSIZE, stdin);
        if (linelen == -1)
        {
            printf("error: length of input string ≥ %d\n", BUFSIZE);
        }

        if (linelen > 0)
        {
            break;
        }
    }

    //---------------------------------------------
    // 2: parse input into array of string tokens
    //---------------------------------------------
    int tokMaxLen = maxTokenLen(buffer, linelen);
    int tokMinLen = minTokenLen(buffer, linelen);
    *len = linelen/tokMinLen + 1;
    char **toks = (char **)calloc(*len, sizeof(char *));

    int tokCount = 0;
    char *str = strtok(buffer, " \t");
    while (1)
    {
        if (!str)
        {
            break;
        }

        *(toks + tokCount) = (char *)calloc(tokMaxLen + 1, sizeof(char));
        strcpy(*(toks + tokCount), str);
        tokCount++;
        str = strtok(0, " \t");
    }

    *n_tokens = tokCount;
    return toks;
}

void freeCmdTokens(char **tokens, int n_tokens, int len)
{
    // free tokens
    for (int i = 0; i < n_tokens; i++)
    {
        free(*(tokens + i));
    }

    // free array holding tokens
    free(tokens);
}
