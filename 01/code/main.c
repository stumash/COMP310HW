// std
#include <stdio.h>
#include <stdlib.h>

// syscall
#include <sys/types.h>
#include <unistd.h>

// c99 types
#include <inttypes.h>

// my code
#include "mygetline/mygetline.h"

// constants
#define BUFSIZE 256

int main()
{
    char buffer[BUFSIZE];
    size_t length = mygetline(buffer, BUFSIZE, stdin);
    printf("'%s' is %d characters long\n", buffer, (int)length);
    return 0;
}
