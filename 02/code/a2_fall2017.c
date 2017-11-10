#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <semaphore.h>

#include <utils/getCmdTokens.h>

#define MAX_RES_NAME_LEN 256 // max numbers of chars in name of reservation
#define N_TABLES 20 // number of tables at restaurant (tables 100-109,200-209)

// cmd strings
#define RESERVE "reserve"
#define INIT "init"
#define STATUS "status"
#define EXIT "exit"

void exit_gracefully(int signo); // signal handler for SIGINT a.k.a. ^C

int shm_fd; // shared memory object file descriptor
char *shm_addr; // address of shared memory in mapping to virutal memory
struct stat s; // file info struct for storing diagnostics on shm



int main()
{
    // setup signal handler
    if (signal(SIGINT, exit_gracefully) == SIG_ERR)
    {
        printf("Error setting signal handler\n");
        exit(1);
    }

    // setup shared memory
    shm_fd = shm_open("/smasha2_a2_shm", O_RDWR | O_CREAT, 0664);
    if (shm_fd == -1)
    {
        printf("Error opening shm\n");
        printf("%s\n", strerror(errno));
        exit(1);
    }
    if (fstat(shm_fd, &s) == -1)
    {
        printf("Error fstat\n");
        exit(1);
    }

    ftruncate(shm_fd, MAX_RES_NAME_LEN * N_TABLES);
    if (fstat(shm_fd, &s) == -1)
    {
        printf("ftruncate failed\n");
        exit(1);
    }

    // addr in mem to write to, length to read from fd,
    // memory page permissions, shared or private mapping to memory,
    // shm filedesc, offset
    shm_addr = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, shm_fd, 0);

    // main loop
    char *shmad = shm_addr; // temp variable
    while (1)
    {
        int ntokens = 0;
        int tokenlen = 0;
        char **tokens = getCmdTokens(&ntokens, &tokenlen);

        // do the shizz
        if (strcmp(tokens[0], RESERVE))
        {
            //
        }
        else if (strcmp(tokens[0], INIT))
        {
            //
        }
        else if (strcmp(tokens[0], STATUS))
        {

        }
        else if (strcmp(tokens[0], EXIT))
        {
            printf("shutting down\n");
        }
        else // bad input
        {
            printf("%s: no such command\n", tokens[0]);
        }

        freeCmdTokens(tokens, ntokens, tokenlen);
    }
}

void exit_gracefully(int signo)
{
    close(shm_fd);
    write(STDOUT_FILENO, shm_addr, s.st_size);
    exit(0);
}
