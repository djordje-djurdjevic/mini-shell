#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>      // fork, close, dup2, execvp, _exit
#include <stdio.h>       // perror
#include <sys/wait.h>    // wait, WIFEXITED, WEXITSTATUS

#include "executor.h"

bool run_program(char **args, int fd, int target_fd)
{
    // CHILD PROCESS
    pid_t pid = fork();

    if (pid == -1)
    {
        if (fd != -1)
            close(fd);
        perror("fork");
        // return false;
    }
    else if (pid == 0)
    { // this does the child process
        if (fd != -1)
        {
            dup2(fd, target_fd);
        }

        execvp(args[0], args);

        // if it gets here it means it failed
        if (fd != -1)
            close(fd);
        _exit(127);
    }
    else
    {
        if (fd != -1)
        {
            close(fd);
        }

        int status;
        wait(&status);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
        {
            return false;
        }
    }

    return true;
}
