#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "common.h"
#include "pipe.h"
#include "redirect.h"

bool execute_pipe(Pipeline pipeline, bool is_background) 
{
    bool no_error = true;
    int pipes[pipeline.num_of_commands - 1][2];
    pid_t pids[pipeline.num_of_commands];

    for(int i = 0; i < pipeline.num_of_commands - 1; i++)
    {
        pipe(pipes[i]);
    }

    for(int i = 0; i < pipeline.num_of_commands; i++)
    {
        pids[i] = fork();
        
        if (pids[i] == -1)
        {
            perror("fork");
        }
        else if (pids[i] == 0)
        { // this does the child process

            if(i > 0)
            {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if(i < pipeline.num_of_commands - 1)
            {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            else {
                int target_fd;
                int fd = check_output_redirect(pipeline.command[i].args, &target_fd);
                if (fd != -1) 
                {
                    dup2(fd, target_fd);
                }
            }


            for(int k = 0; k < pipeline.num_of_commands - 1; k++)
            {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            execvp(pipeline.command[i].args[0], pipeline.command[i].args);

            // if it gets here it means it failed
            _exit(127);
        }
        else
        {   
            //do nothing
        }
    }

    for(int i = 0; i < pipeline.num_of_commands - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for(int i = 0; i < pipeline.num_of_commands; i++)
    {
        int status;
        if (!is_background)
        {
            waitpid(pids[i], &status, 0);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
        {
            no_error = false;
        }
    }

    if(is_background)
    {
        char command_buf[MAX_SIZE] = "";

        for(int i = 0; i < pipeline.num_of_commands; i++)
        {
            for (int j = 0; pipeline.command[i].args[j] != NULL; j++) 
            {
                strcat(command_buf, pipeline.command[i].args[j]);
                if (pipeline.command[i].args[j + 1] != NULL) 
                {
                    strcat(command_buf, " ");
                }
            }

            if(i < pipeline.num_of_commands - 1) 
            {
                strcat(command_buf, " | ");
            }
        }
        strcat(command_buf, " &");

        if (job_count == 0)
        {
            next_job_number = 1;
        }

        jobs[job_count++] = (Job){ .job_number = next_job_number++, .pid = pids[pipeline.num_of_commands - 1], .command = strdup(command_buf)};
        snprintf(jobs[job_count-1].status, STATUS_LEN, "%-24s", "Running");            
        
        printf("[%d] %d\n",next_job_number - 1, pids[pipeline.num_of_commands - 1]);
    }

    return no_error;
}