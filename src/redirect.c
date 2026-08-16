#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>      // fork, close, dup2, execvp, _exit

#include "redirect.h"


int check_output_redirect(char **args, int *target_fd)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if ((strcmp(args[i], ">") == 0 || strcmp(args[i], "1>") == 0) && args[i + 1] != NULL)
        {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // read only, create if doesnt exist, truncate if it exists, para for create

            if (fd == -1)
            {
                fprintf(stderr, "%s: %s\n", args[i + 1], strerror(errno));
                return fd;
            }

            args[i] = NULL;
            args[i + 1] = NULL;

            *target_fd = 1; // stdout
            return fd;
        }
        else if ((strcmp(args[i], "2>") == 0) && args[i + 1] != NULL)
        {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // read only, create if doesnt exist, truncate if it exists, para for create

            if (fd == -1)
            {
                fprintf(stderr, "%s: %s\n", args[i + 1], strerror(errno));
                return fd;
            }

            args[i] = NULL;
            args[i + 1] = NULL;

            *target_fd = 2; // stderr
            return fd;
        }
        else if ((strcmp(args[i], ">>") == 0 || strcmp(args[i], "1>>") == 0) && args[i + 1] != NULL)
        {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644); // read only, create if doesnt exist, truncate if it exists, para for create

            if (fd == -1)
            {
                fprintf(stderr, "%s: %s\n", args[i + 1], strerror(errno));
                return fd;
            }

            args[i] = NULL;
            args[i + 1] = NULL;

            *target_fd = 1; // stdout
            return fd;
        }
        else if ((strcmp(args[i], "2>>") == 0) && args[i + 1] != NULL)
        {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644); // read only, create if doesnt exist, truncate if it exists, para for create

            if (fd == -1)
            {
                fprintf(stderr, "%s: %s\n", args[i + 1], strerror(errno));
                return fd;
            }

            args[i] = NULL;
            args[i + 1] = NULL;

            *target_fd = 2; // stderr
            return fd;
        }
    }
    return -1;
}

void restore_std(int fd, int saved_std, int target_fd)
{
    if (saved_std != -1)
    {
        dup2(saved_std, target_fd);
        close(fd);
        close(saved_std);
    }
}