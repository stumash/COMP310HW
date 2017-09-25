// std
#include <stdio.h>
#include <stdlib.h>

// syscall
#include <sys/types.h>
#include <unistd.h>


// my helper functions
#include "utils/getLine.h"
#include "utils/minTokenLen.h"
#include "utils/maxTokenLen.h"

// constants
#define BUFSIZE 256

int main()
{
    char buffer[BUFSIZE];
    while (1)
    {
        ssize_t linelen = getLine(buffer, BUFSIZE, stdin);

        if (linelen == 0) {
            continue;
        } else if (linelen < 0) {
            printf("command-string too long. 256 chars max.");
            continue;
        }

        ssize_t minwlen = minTokenLen(buffer, linelen);
        ssize_t maxwlen = maxTokenLen(buffer, linelen);
        int maxtknqty = linelen / (minwlen + 1);

        char *tokens[maxtknqty];
        for (int i = 0; i < maxtknqty; i++)
        {
            tokens[i] = calloc(maxwlen, sizeof(char));
        }
    }

    return 0;
}
