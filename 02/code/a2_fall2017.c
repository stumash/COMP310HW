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
#include <math.h>

#include <semaphore.h>

#include <utils/getCmdTokens.h>

// shared memory object format
#define MAX_RES_NAME_LEN 256 // max numbers of chars in name of reservation
#define N_TABLES 20 // number of tables at restaurant (tables 100-109,200-209)

// shared memory name
#define RESERVATIONS "/smasha2_a2_shm"
#define READERCOUNT "/smasha2_a2_rc"

// cmd strings
#define RESERVE "reserve"
#define INIT "init"
#define STATUS "status"
#define EXIT "exit"

// semaphore names
#define SEM_WRITE "sem_write" // semaphore to alter number of active writers (0 or 1)
#define SEM_READERCOUNT "sem_readercount" // semaphore to alter number of active readers
// semaphores
sem_t *write_mutex;
sem_t *readercount_mutex;

// helper functions
void exit_gracefully(int signo); // signal handler for SIGINT a.k.a. ^C
int tableNumFromIndex(int idx); // map 0->100, 1->101, ... 10->200, 11->201
int isInt(char *str); // return 1 is string is int
int strToInt(char *str); // convert string to int

// shm for reservations
int shm_fd; // shared memory object file descriptor (reservations)
char *shm_addr; // address of shared memory in mapping to virutal memory (reservations)
struct stat s; // file info struct for storing diagnostics on shm (reservations)

// shm for readercount
int shm_rc_fd; // shared memory object file descriptor (readercount)
int *shm_rc_addr; // address of shared memory in mapping to virutal memory (readercount)
struct stat s_rc; // file info struct for storing diagnostics on shm (readercount)

int main()
{
    // setup signal handler
    if (signal(SIGINT, exit_gracefully) == SIG_ERR)
    {
        printf("Error setting signal handler\n");
        exit(1);
    }

    // setup shared memory for reservations
    shm_fd = shm_open(RESERVATIONS, O_RDWR | O_CREAT, 0664);
    if (shm_fd == -1)
    {
        printf("Error opening shm (reservations)\n");
        printf("%s\n", strerror(errno));
        exit(1);
    }
    if (fstat(shm_fd, &s) == -1)
    {
        printf("Error fstat (reservations)\n");
        exit(1);
    }
    ftruncate(shm_fd, MAX_RES_NAME_LEN * N_TABLES);
    if (fstat(shm_fd, &s) == -1)
    {
        printf("ftruncate failed (reservations)\n");
        exit(1);
    }
    shm_addr = mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // setup shared memory for readercount
    shm_rc_fd = shm_open(READERCOUNT, O_RDWR | O_CREAT, 0664);
    if (shm_rc_fd == -1)
    {
        printf("Error opening shm (readercount)\n");
        printf("%s\n", strerror(errno));
        exit(1);
    }
    if (fstat(shm_rc_fd, &s_rc) == -1)
    {
        printf("Error fstat (readercount)\n");
        exit(1);
    }
    ftruncate(shm_rc_fd, sizeof(int));
    if (fstat(shm_rc_fd, &s_rc) == -1)
    {
        printf("ftruncate failed (readercount)\n");
        exit(1);
    }
    shm_rc_addr = mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_rc_fd, 0);

    // setup semaphores
    write_mutex = sem_open(SEM_WRITE, O_CREAT, 0644, 1);
    readercount_mutex = sem_open(SEM_READERCOUNT, O_CREAT, 0644, 1);

    // main loop
    char *shmad = shm_addr; // temp variable
    while (1)
    {
        int ntokens = 0;
        int tokenlen = 0;
        char **tokens = getCmdTokens(&ntokens, &tokenlen);

        //
        // RESERVE
        //
        if (!strcmp(tokens[0], RESERVE))
        {
            sem_wait(write_mutex);

            int secnum;
            // validate 'section number' argument
            if (!(tokens[2]))
            {
                printf("You must supply a section as 'A' or 'B'.\n");
                continue;
            }
            if (strcmp(tokens[2], "A") != 0 && strcmp(tokens[2], "B") != 0)
            {
                printf("You must supply a section as 'A' or 'B'.\n");
                continue;
            }
            if (strcmp(tokens[2], "A") == 0)
            {
                secnum = 0;
            }
            else // "B"
            {
                secnum = 1;
            }

            // parse 'table number' argument
            int tablenum = -1;
            int idx = 0;
            if (tokens[3])
            {
                if (isInt(tokens[3]))
                {
                    tablenum = strToInt(tokens[3]);
                    if (tablenum < 100 || (tablenum > 109 && tablenum < 200) || tablenum > 209)
                    {
                        printf("The table number argument must be an int in range [100-109] or [200-209]\n");
                        continue;
                    }
                    if (!(tablenum / 100 - 1 + 'A' == tokens[2][0]))
                    {
                        printf("The table number must be in the correct section\n");
                        continue;
                    }
                    idx = (tablenum % 100) + (((tablenum / 100) - 1) * 10);
                }
                else
                {
                    printf("The table number argument must be an int in range [100-109] or [200-209]\n");
                    continue;
                }
            }

            // parse 'name' argument and save reservation
            if (tablenum == -1)
            {
                int foundSlot = 0;
                for (int i = secnum * 10; i < secnum*10 + 10; i++)
                {
                    if (!shm_addr[i * MAX_RES_NAME_LEN])
                    {
                        foundSlot = 1;
                        strcpy(shm_addr + i * MAX_RES_NAME_LEN, tokens[1]);
                    }
                    if (foundSlot)
                        break;
                }

                if (!foundSlot)
                {
                    printf("No tables available in that section\n");
                    continue;
                }
            }
            else // it was passed as arg
            {
                if (*(shm_addr + idx * MAX_RES_NAME_LEN))
                {
                    printf("table unavailable\n");
                    continue;
                }
                strcpy(shm_addr + idx * MAX_RES_NAME_LEN, tokens[1]);
            }

            sem_post(write_mutex);
        }

        //
        // INIT
        //
        else if (!strcmp(tokens[0], INIT))
        {
            sem_wait(write_mutex);

            for (int i = 0; i < N_TABLES * MAX_RES_NAME_LEN; i++)
            {
                shm_addr[i] = 0;
            }

            sem_post(write_mutex);
        }

        //
        // STATUS
        //
        else if (!strcmp(tokens[0], STATUS))
        {
            sem_wait(readercount_mutex);
            *shm_rc_addr = *shm_rc_addr + 1;
            if (*shm_rc_addr == 1) { sem_wait(write_mutex); }
            sem_post(readercount_mutex);


            int isEmpty = 1;
            for (int i = 0; i < N_TABLES; i++)
            {
                int j = i * MAX_RES_NAME_LEN;
                if (*(shm_addr + j))
                {
                    isEmpty = 0;
                    printf("%d %s\n", tableNumFromIndex(i), shm_addr + j);
                }
            }
            if (isEmpty)
            {
                printf("No reservations yet\n");
            }

            sem_wait(readercount_mutex);
            *shm_rc_addr = *shm_rc_addr - 1;
            if (*shm_rc_addr == 0) { sem_post(write_mutex); }
            sem_post(readercount_mutex);
        }

        //
        // EXIT
        //
        else if (!strcmp(tokens[0], EXIT))
        {
            break; // break the loop, go to exit_gracefully(0)
        }

        //
        // BAD INPUT
        //
        else // bad input
        {
            printf("%s: no such command\n", tokens[0]);
        }

        freeCmdTokens(tokens, ntokens, tokenlen);
    }

    exit_gracefully(0);
}

/**
 * Release semaphores, release the shm, write its contents to stdout, exit
 */
void exit_gracefully(int signo)
{
    sem_close(write_mutex);
    sem_close(readercount_mutex);

    close(shm_fd);
    close(shm_rc_fd);
    write(STDOUT_FILENO, shm_addr, s.st_size);
    write(STDOUT_FILENO, shm_rc_addr, s.st_size);
    exit(0);
}

int tableNumFromIndex(int idx)
{
    if (idx < 10)
    {
        return 100 + idx;
    }
    else if (idx < 20)
    {
        return 200 + (idx % 10);
    }
    else
    {
        return -1;
    }
}

int isInt(char *str)
{
    int i = 0;
    while (str[i])
    {
        if (str[i] < '0' || str[i] > '9')
        {
            return 0;
        }
        i++;
    }
    return 1;
}


int strToInt(char *str)
{
    int len = strlen(str);
    int num = 0;
    for (int i = len-1; i >= 0; i--)
    {
        num += (str[i] - '0') * pow(10, len-1-i);
    }
}
