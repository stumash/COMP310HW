// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// my helper functions
#include <utils/getCmdTokens.h>

// constants
#include <utils/constants.h>

// syscall
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

// user input token parsing variables/setup
char **tokens; // tokens parsed by getCmdTokens()
int n_tokens; // number of tokens parsed by getCmdTokens()
int len; // length of tokens array
int bg; // 1 if last token is "&", else 0

// array of child process ids
int *prockids;
// pid of foreground process
int fpid;

//-----------------------------
// SIGNAL HANDLER
//-----------------------------
static void
sig_handler(int signum)
{
    if (signum == SIGTSTP)
    {
        exit(0); //TODO delete this line TODO
        // do nothing
    }
    else // signum must be SIGINT
    {
        // if there is a forgrounded process
        if (fpid > 0)
        {
            kill(fpid, SIGKILL); // kill it
        }
        printf("\n");
        fpid = 0;
    }
}

//-----------------------------
// MAIN
//-----------------------------
int main()
{
    // initialize globals
    prockids = (int *)calloc(MAX_CHILD_PROC, sizeof(int));
    fpid = 0;

    // signal handling variables/setup
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = &sig_handler;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        fprintf(stderr, "SIGINT (^C) sigaction failed");
        return 1;
    } if (sigaction(SIGTSTP, &act, NULL) < 0) {
        fprintf(stderr, "SIGTSTP (^Z) sigaction failed");
        return 1;
    }

    while (1)
    {
        int pstatus = 0;
        for (int i = 0; i < MAX_CHILD_PROC; i++) {
            if (*(prockids + i) != 0) {
                if (waitpid(*(prockids + i), &pstatus, WNOHANG)) {
                    if (WIFEXITED(pstatus) || WIFSIGNALED(pstatus) || WCOREDUMP(pstatus)) {
                        *(prockids + i) = 0;
                    }
                }
            }
        }

        bg = 0;
        tokens = getCmdTokens(&n_tokens, &len);

        // if should run command in background
        if (strcmp("&", *(tokens + n_tokens-1)) == 0)
        {
            bg = 1;
            free(*(tokens + n_tokens-1));
            *(tokens + n_tokens-1) = 0;
            n_tokens--;
        }

        // fork process to run command
        int pid = (int)fork();
        // fork fails
        if (pid < 0)
        {
            printf("error: fork failed\n");
        }
        // after fork, in parent process
        else if (pid > 0)
        {
            if (!bg)
            {
                fpid = pid;
                waitpid(pid, 0, 0);
                fpid = 0;
            }
            else
            {
                // add pid to list of background processes
                for (int i = 0; i < MAX_CHILD_PROC; i++)
                {
                    if (*(prockids + i) == 0)
                    {
                        *(prockids + i) = pid;
                        break;
                    }
                }
            }

            freeCmdTokens(tokens, n_tokens, len);
        }
        // after fork, in child process
        else
        {
            if (strcmp(*(tokens), "jobs") == 0)
            {
                for (int i = 0; i < MAX_CHILD_PROC; i++)
                {
                    if (*(prockids + i) != 0)
                    {
                        printf("%d\n", *(prockids + i));
                    }
                }
                return 0;
            }

            if (n_tokens - 2  > 0 && strcmp(">", *(tokens + n_tokens-2)) == 0)
            {
                fclose(stdout);
                open(*(tokens + n_tokens-1), O_WRONLY|O_CREAT, 0644);

                free(*(tokens + n_tokens - 1));
                free(*(tokens + n_tokens - 2));
                *(tokens + n_tokens - 1) = 0;
                *(tokens + n_tokens - 2) = 0;
                n_tokens -= 2;
            }

            execvp(*tokens, tokens);

            return 0;
        }
    }

    free(prockids);
    return 0;
}
