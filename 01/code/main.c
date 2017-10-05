// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// syscall
#include <sys/types.h>
#include <unistd.h>

// my helper functions
#include <utils/getCmdTokens.h>

int main()
{
    char **tokens; // tokens parsed by getCmdTokens()
    int n_tokens; // number of tokens parsed by getCmdTokens()
    int len; // length of tokens array
    int bg; // 1 if last token is "&", else 0
    while (1)
    {
        bg = 0;
        tokens = getCmdTokens(&n_tokens, &len);
        if (strcmp("&", *(tokens + n_tokens-1)) == 0)
            bg = 1;

        int pid = (int)fork();
        if (pid < 0)
        {
            printf("error: fork failed\n");
        }
        else if (pid > 0) // in parent process
        {
            printf("in parent\n");
            if (!bg)
            {
                // TODO
            }
            else
            {
                // TODO
            }
        }
        else // in child process
        {
            execvp(*tokens, tokens + 1);
            exit(0);
        }


        freeCmdTokens(tokens, n_tokens, len);
    }

    return 0;
}
