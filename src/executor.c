#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>      // fork, close, dup2, execvp, _exit
#include <stdio.h>       // perror
#include <sys/wait.h>    // wait, WIFEXITED, WEXITSTATUS
#include <string.h>
#include <stdlib.h>

#include "executor.h"
#include "common.h"

int job_count = 0;
int next_job_number = 1;
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
            strcat(command_buf, " &");

            if (job_count == 0)
            {
                next_job_number = 1;
            }

            jobs[job_count++] = (Job){ .job_number = next_job_number++, .pid = pid, .command = strdup(command_buf)};
            snprintf(jobs[job_count-1].status, STATUS_LEN, "%-24s", "Running");            
            
            printf("[%d] %d\n",next_job_number-1 ,pid);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
        {
            return false;
        }
    }

    return true;
}

void mark_job_as_done()
{
    //printf("DEBUG: cleanup called\n");
    
    pid_t finished_pid;
    int status;

    while ( (finished_pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for (int i = 0; i < job_count; i++)
        {
            if (jobs[i].pid == finished_pid)
            {
                snprintf(jobs[i].status, STATUS_LEN, "%-24s", "Done");

                int len = strlen(jobs[i].command); 
                if(len > 2) 
                {
                    jobs[i].command[len - 2] = '\0';
                }
                break;
            }
        }
    }
}