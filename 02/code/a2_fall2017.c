#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

void exit_gracefully(int signo); // signal handler for SIGINT a.k.a. ^C

int shm_fd; // shared memory object file descriptor
char *shm_addr; // address of shared memory in mapping to virutal memory
struct stat s; // file info struct for storing diagnostics on shm
char cmd_buf[256]; // buffer to store user commands

int main()
{
    if (signal(SIGINT, exit_gracefully) == SIG_ERR)
    {
        printf("Error setting signal handler\n");
        exit(1);
    }

    shm_fd = shm_open("smasha2_a2_shm", O_RDWR, 0);
    if (shm_fd < 0)
    {
        printf("Error opening shm\n");
        exit(1);
    }
    if (fstat(shm_fd, &s) == -1)
    {
        printf("Error fstat\n");
    }
    shm_addr = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, shm_fd, 0);

}

void exit_gracefully(int signo)
{
    close(shm_fd);
    write(STDOUT_FILENO, shm_addr, s.st_size);
}
