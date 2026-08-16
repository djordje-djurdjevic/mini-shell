#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#include "redirect.h"
#include "common.h"
#include "builtins.h"

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

    bool is_builtin = (strcmp(args[0], "exit") == 0 ||
                       strcmp(args[0], "echo") == 0 ||
                       strcmp(args[0], "type") == 0 ||
                       strcmp(args[0], "pwd") == 0 ||
                       strcmp(args[0], "cd") == 0);

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
    char *builtins[] = {"echo", "exit", "type", "pwd", "cd"};
    int length = sizeof(builtins) / sizeof(builtins[0]);

    for (int i = 0; i < length; i++)
    {
        if (strcmp(builtins[i], input) == 0)
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