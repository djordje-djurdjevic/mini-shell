#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>      // malloc, realloc
#include <string.h>      // strcpy, strlen

#include "common.h"
#include "parser.h"


Pipeline parse_input(char *input, bool *is_background)
{
    int capacity = 10;
    int num_of_args = 0;
    char **arguments = malloc(capacity * sizeof(char *));

    char const single_qoute = '\'';
    char const double_qoute = '\"';
    char const backslash = '\\';

    bool in_single_quotes = false;
    bool in_double_quotes = false;

    char arg[MAX_SIZE];
    int i_arg = 0;

    for (int i = 0; input[i] != '\0'; i++)
    {

        // Backslash behavior depends on context (outside quotes / double / single).
        // (single_quotes not handled here - backslash inside '...' is fully literal,
        // caught by the plain-character branch below via the !in_single_quotes guard.)
        if (input[i] == backslash && !in_single_quotes)
        {
            // Inside "..." backslash only escapes: \ " $ `
            // Anything else stays literal and gets handled normally next iteration.
            if (in_double_quotes)
            {
                if (input[i + 1] == '\\' ||
                    input[i + 1] == '\"' ||
                    input[i + 1] == '$' ||
                    input[i + 1] == '`')
                {
                    arg[i_arg++] = input[++i];
                }
                else
                {
                    arg[i_arg++] = input[i];
                }
            }
            else
            {
                if (input[i + 1] != '\0') 
                {
                    arg[i_arg++] = input[++i];
                }
                else 
                {
                    // trailing backslash - nothing to escape, avoid reading stale buffer data
                    break;
                }
            }

            continue;
        }

        if (input[i] == single_qoute && !in_double_quotes)
        { // inside double quotes treated like literal
            in_single_quotes = !in_single_quotes;
            continue;
        }
        else if (input[i] == double_qoute && !in_single_quotes)
        {
            in_double_quotes = !in_double_quotes;
            continue;
        }

        if (input[i] == ' ' && !in_single_quotes && !in_double_quotes)
        {
            if (i_arg > 0)
            {

                arg[i_arg] = '\0';
                arguments[num_of_args] = malloc(strlen(arg) + 1); // strdup does this line and line below
                strcpy(arguments[num_of_args], arg);
                num_of_args++;
                strcpy(arg, "");
                i_arg = 0;

                if (num_of_args >= capacity - 1)
                {
                    capacity *= 2;
                    arguments = realloc(arguments, capacity * sizeof(char *));
                }
            }

            continue;
        }

        arg[i_arg++] = input[i];
    }

    if (i_arg > 0)
    {
        arg[i_arg] = '\0';
        arguments[num_of_args] = malloc(strlen(arg) + 1); // strdup does this line and line below
        strcpy(arguments[num_of_args], arg);
        num_of_args++;
    }

    // for (int i = 0; i < num_of_args; i++) {
    //     printf("%s|\n", arguments[i]);
    // }
    if (strcmp(arguments[num_of_args - 1], "&") == 0)
    {
        *is_background = true;
        arguments[num_of_args - 1] = NULL;
    }
    else
    {
        *is_background = false;
        arguments[num_of_args] = NULL;
    }

    Pipeline pipeline;
    pipeline.num_of_commands = 0;
    pipeline.command[0].args = malloc(capacity * sizeof(char *));

    num_of_args = 0;
    capacity = 2;

    
    for (int i = 0; arguments[i] != NULL; i++)
    {
        if(strcmp(arguments[i], "|") == 0)
        {   
            pipeline.command[pipeline.num_of_commands].args[num_of_args] = NULL;        
            pipeline.command[++pipeline.num_of_commands].args = malloc(capacity * sizeof(char *));
            num_of_args = 0;
            capacity = 2;
        }
        else
        {
            pipeline.command[pipeline.num_of_commands].args[num_of_args++] = arguments[i];        
        }

        if (num_of_args >= capacity - 1)
        {
            capacity *= 2;
            pipeline.command[pipeline.num_of_commands].args  = realloc(
            pipeline.command[pipeline.num_of_commands].args , capacity * sizeof(char *));
        }
    }

    pipeline.command[pipeline.num_of_commands].args[num_of_args] = NULL;     
    pipeline.num_of_commands++; // "| + 1"

    free(arguments);
    return pipeline;
}

void free_commands(Pipeline pipeline)
{
    for (int i = 0; i < pipeline.num_of_commands; i++)
    {
        for (int j = 0; pipeline.command[i].args[j] != NULL; j++)
        {
            free(pipeline.command[i].args[j]);
        }
        free(pipeline.command[i].args);
    }
}

int parse_escape_sequence(char *input, int i)
{
    char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return i;
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return i;

    if (seq[0] != '[') 
    {
        return i;
    }
    if (seq[1] == 'A') // up arrow ESC [ A
    {
        if(history_position == 0)
        {
            printf("\a");
            return i;
        }

        if (command_counter > 0 && history_position > 0) 
        {
            i = up_arrow(input, i);
            //printf("DEBUG: command counter: %d, hist_pos: %d\n", command_counter, history_position)
        }
    }
    else if (seq[1] == 'B')
    {
        if(history_position == command_counter)
        {
            printf("\a");
            return i;
        }

        if(history_position == command_counter - 1)
        {   
            for(int j = i; j > 0; j--)
            {
                i = backspace(input, i);
            }
            strcpy(input, "");
            history_position++;

            return i;
        }

        if (command_counter > 0 && history_position != command_counter - 1) 
        {
            i = down_arrow(input, i);
        }
    }

    return i;
}

int up_arrow(char *input, int i)
{
    for(int j = i; j > 0; j--)
    {
        i = backspace(input, i);
    }

    history_position--;
    printf("%s", command_history[history_position]);
    fflush(stdout);
    strcpy(input, command_history[history_position]);
    
    return strlen(command_history[history_position]);
}

int down_arrow(char *input, int i)
{
    for(int j = i; j > 0; j--)
    {
        i = backspace(input, i);
    }
    
    history_position++;
    printf("%s", command_history[history_position]);
    fflush(stdout);
    strcpy(input, command_history[history_position]);
    
    return strlen(command_history[history_position]);
}

int backspace(char *input, int i)
{
    i--;
    input[i] = '\0';
    printf("\b \b");
    fflush(stdout);

    return i;
}