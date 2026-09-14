#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "redirect.h"
#include "common.h"
#include "builtins.h"
#include "completion.h"


const char *BUILTINS[] = {"echo", "exit", "type", "pwd", "cd", "complete", "jobs", "history"};
const int BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(BUILTINS[0]);

#define MAX_COMPLETIONS 32
char registered_commands[MAX_COMPLETIONS][MAX_SIZE];
char registered_paths[MAX_COMPLETIONS][MAX_SIZE];
int registered_count = 0;

bool echo(char **args)
{

    for (int i = 1; args[i] != NULL; i++)
    {
        printf("%s", args[i]);

        if (args[i + 1] != NULL)
            printf(" ");
    }

    printf("\n");
    return true;
}

bool run_builtin(char **args, int fd, int target_fd)
{

    bool is_builtin = false;
    for(int i = 0; i < BUILTINS_COUNT; i++) 
    {
        if (strcmp(args[0], BUILTINS[i]) == 0) 
        {
            is_builtin = true;
            break;
        }
    }

    if (!is_builtin)
    {
        return false; // fd is untouched, ProgramFunction can use it
    }

    bool result = false;

    int saved_std = -1;
    if (fd != -1)
    {                               // fd = fd of file
        saved_std = dup(target_fd); // target fd 1 or 2 
        dup2(fd, target_fd);        
    }


    //typedef bool (*builtin_fn)(char **args);
    if (strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }
    else if (strcmp(args[0], "echo") == 0)
    {
        result = echo(args);
    }
    else if (strcmp(args[0], "type") == 0)
    {
        result = type(args[1]);
    }
    else if (strcmp(args[0], "pwd") == 0)
    {
        result = pwd();
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        result = cd(args[1]);
    }
    else if (strcmp(args[0], "complete") == 0)
    {
        result = complete(args);
    }
    else if (strcmp(args[0], "jobs") == 0)
    {
        result = background_jobs();
    }
    else if (strcmp(args[0], "history") == 0)
    {
        result = history(args);
    }


    restore_std(fd, saved_std, target_fd);
    return result;
}

bool type(char *input)
{

    if (input == NULL)
    {
        return true;
    }

    // BUILTIN
    for (int i = 0; i < BUILTINS_COUNT; i++)
    {
        if (strcmp(input, BUILTINS[i]) == 0)
        {
            printf("%s is a shell builtin\n", input);
            return true;
        }
    }

    // CHECKING PATH env var
    char *path_env = getenv("PATH"); 
    char path_env_cpy[MAX_SIZE];
    strcpy(path_env_cpy, path_env);

    char *all_paths = strtok(path_env_cpy, ":");

    while (all_paths != NULL)
    {
        char full_path[MAX_SIZE];

        snprintf(full_path, sizeof(full_path), "%s/%s", all_paths, input);

        if (access(full_path, F_OK) == 0 && access(full_path, X_OK) == 0)
        {
            printf("%s is %s\n", input, full_path);
            return true;
        }

        all_paths = strtok(NULL, ":");
    }
    printf("%s: not found\n", input);

    return true;
}


bool pwd()
{
    char *cwd = getcwd(NULL, 0);

    if (cwd != NULL)
    {
        printf("%s\n", cwd);
        free(cwd);
        return true;
    }

    perror("getcwd() error");
    return false;
}

bool cd(char *input)
{

    if (input == NULL)
    {
        fprintf(stderr, "cd: missing argument\n");
        return true;
    }

    if (strcmp(input, "~") == 0)
    {
        input = getenv("HOME");
    }

    if (chdir(input) == 0)
    { // works on relative paths aswell
    }
    else
    {
        fprintf(stderr, "cd: %s: No such file or directory\n", input);
    }

    return true;
}

bool complete(char **args) {

    if(args[1] == NULL) 
    {
        return true;
    }


    if(strcmp(args[1], "-p") == 0)
    {   
        if(args[2] == NULL) 
        {
            return true;
        }
       
        for (int i = 0; i < registered_count; i++) 
        {
            if (strcmp(args[2], registered_commands[i]) == 0)
            {
                printf("complete -C '%s' %s\n", registered_paths[i], registered_commands[i]);
                return true;
            }
        }

        printf("complete: %s: no completion specification\n", args[2]);
        return true;
    }
    else if (strcmp(args[1], "-C") == 0) 
    {   
        if(args[2] == NULL || args[3] == NULL) 
        {
            return true;
        }

        for (int i = 0; i < registered_count; i++) 
        {
            if (strcmp(args[3], registered_commands[i]) == 0)
            {
                strcpy(registered_paths[i], args[2]); //update
                //strcpy(registered_commands[i], args[3]);   not necessesary   
                return true;  
            }
        }   
        strcpy(registered_paths[registered_count], args[2]);
        strcpy(registered_commands[registered_count], args[3]);
        registered_count++;
    }
    else if(strcmp(args[1], "-r") == 0) 
    {
        if(args[2] == NULL) 
        {
            return true;
        }

        for (int i = 0; i < registered_count; i++) 
        {
            if (strcmp(args[2], registered_commands[i]) == 0)
            {
                strcpy(registered_commands[i], registered_commands[registered_count-1]);
                strcpy(registered_paths[i]   , registered_paths[registered_count-1]);
                registered_count--;
                break;
            }
        }
    }

    return true;
}

bool background_jobs()
{
    print_job_status("", 0); //len zero means show all
    remove_done_jobs();

    return true;
}

void print_job_status(char *compare_for_output, int len_cmp_for_output)
{
    char prefix = ' ';

    if (job_count == 0) 
    {
        return;
    }
    for (int i = 0; i < job_count; i++)
    {   
        if (i == job_count - 1)
        {
            prefix = '+';
        }
        else if (i == job_count - 2)
        {
            prefix = '-';
        }
        else 
        {
            prefix = ' ';
        }

        if(strncmp(jobs[i].status, compare_for_output, len_cmp_for_output) == 0)
        {
            printf("[%d]%c  %s %s\n", jobs[i].job_number, prefix, jobs[i].status, jobs[i].command);
        }
    }
}

void remove_done_jobs()
{
    for (int i = job_count - 1; i >= 0; i--)
    {
        if(strncmp(jobs[i].status, "Done", 4) == 0)
        {
            free(jobs[i].command);
            for (int j = i; j < job_count-1 ; j++) 
            {
                jobs[j] = jobs[j+1];
            }
            job_count--;
        }
    }
}

bool history(char **args)
{
    int start;

    if(args[1] == NULL)
    {
        start = 0;
    }
    else
    {
        char *endptr;
        long val = strtol(args[1], &endptr, 10);
    
        if (*endptr != '\0')
        {
            return true;
        }

        start = command_counter - val;
    }

    for(int i = start; i < command_counter; i++)
    {
        printf("%4d  %s\n", i+1, command_history[i]);
    }

    return true;
}