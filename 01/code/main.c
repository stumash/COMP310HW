// std
#include <stdio.h>
#include <stdlib.h>

// syscall
#include <sys/types.h>
#include <unistd.h>


// my helper functions
#include <utils/getCmdTokens.h>

int main()
{
    while (1)
    {
        char **tokens = 0;
        int tok_len;
        getCmdTokens(tokens, &tok_len);
    }

    return 0;
}
