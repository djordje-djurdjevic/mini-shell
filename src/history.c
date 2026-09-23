#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "history.h"
#include "common.h"


bool history(char **args)
{
    int start;

    if (history_r_flag_helper(args))
    {
        return true;
    }
    else if(history_w_flag_helper(args))
    {
        return true;
    }
    else if(history_a_flag_helper(args))
    {
        return true;
    }

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

bool history_r_flag_helper(char **args)
{

    if(args[1] == NULL)
    {
        return false;
    }
    //printf("DEBUG: Function r flag is called");


    if (strcmp(args[1], "-r") == 0)
    {
        if(args[2] == NULL)
        {
            //printf("DEBUG: arg[2] fail");
            return false;
        }

        FILE *file = fopen(args[2], "r");

        if (file == NULL)
        {
            //printf("DEBUG: invalid file");
            return false;
        }
        //printf("DEBUG: valid file");


        char buffer[MAX_SIZE];
        while(fgets(buffer, sizeof(buffer), file) != NULL)
        {

            if (command_counter >= command_history_capacity)
            {
                command_history_capacity *= 2;
                command_history = realloc(command_history, command_history_capacity * sizeof(char *));
            }
            buffer[strlen(buffer)- 1] = '\0';
            if(strlen(buffer) == 0) 
            {
                continue;
            }

            command_history[command_counter++] = strdup(buffer);
        }

        history_position = command_counter;
        
        fclose(file);
        return true;
    }
    //printf("DEBUG: arg[1] fail");


    return false;
}

bool history_w_flag_helper(char **args)
{
    if(args[1] == NULL)
    {
        return false;
    }
    //printf("DEBUG: Function w flag is called");

    if (strcmp(args[1], "-w") == 0)
    {

        if(args[2] == NULL)
        {
            return false;
        }

        FILE *file = fopen(args[2], "w");
        if (file == NULL)
        {
            return false;
        }

        for(int i = 0;  i < command_counter; i++)
        {
            fprintf(file, "%s\n", command_history[i]);
        }

        fclose(file);
        return true;
    }

    return false;
}


bool history_a_flag_helper(char **args)
{
    if(args[1] == NULL)
    {
        return false;
    }
    //printf("DEBUG: Function a flag is called");

    if (strcmp(args[1], "-a") == 0)
    {

        if(args[2] == NULL)
        {
            return false;
        }

        FILE *file = fopen(args[2], "a");
        if (file == NULL)
        {
            return false;
        }
        //printf("DEBUG: File opened.");


        for(int i = history_append_position;  i < command_counter; i++)
        {
            fprintf(file, "%s\n", command_history[i]);
        }
        history_append_position = command_counter;

        fclose(file);
        return true;
    }

    return false;
}


void write_history_on_exit()
{
    char *temp_args_for_history[3];
    temp_args_for_history[0] = "history";
    temp_args_for_history[1] = "-w";
    temp_args_for_history[2] = getenv("HISTFILE");
    history_w_flag_helper(temp_args_for_history);
}

void read_history_on_start()
{
    char *temp_args_for_history[3];
    temp_args_for_history[0] = "history";
    temp_args_for_history[1] = "-r";
    temp_args_for_history[2] = getenv("HISTFILE");
    history_r_flag_helper(temp_args_for_history);
}