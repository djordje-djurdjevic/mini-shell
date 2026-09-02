#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>      // fork, close, dup2, execvp, _exit
#include <stdio.h>       // perror
#include <sys/wait.h>    // wait, WIFEXITED, WEXITSTATUS
#include <string.h>
#include <stdlib.h>

#include "executor.h"
#include "common.h"

int jobs_count = 0;
int next_jobs_number = 1;
Job jobs[MAX_JOBS];

bool run_program(char **args, int fd, int target_fd, bool is_background)
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

        if (!is_background)
        {
            waitpid(pid, &status, 0);
        }
        else
        {
            char command_buf[MAX_SIZE] = "";

            for (int i = 0; args[i] != NULL; i++) 
            {
                strcat(command_buf, args[i]);
                if (args[i+1] != NULL) 
                {
                    strcat(command_buf, " ");
                }
            }
            jobs[jobs_count++] = (Job){ .job_number = next_jobs_number++, .pid = pid, .command = strdup(command_buf) };
            //dont forget to free
            
            printf("[%d] %d\n",next_jobs_number-1 ,pid);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
        {
            return false;
        }
    }

    return true;
}

void cleanup_finished_jobs()
{
    pid_t finished_pid;
    int status;

    while ( (finished_pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for (int i = 0; i < jobs_count; i++)
        {
            if (jobs[i].pid == finished_pid)
            {
                free(jobs[i].command);
                jobs[i] = jobs[jobs_count-1];
                jobs_count--;
            }
        }
    }
}