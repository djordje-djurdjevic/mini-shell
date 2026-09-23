#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

#include "common.h"
#include "builtins.h"
#include "completion.h"
#include "executor.h"
#include "parser.h"
#include "redirect.h"
#include "pipe.h"
#include "history.h"

void restore_terminal(void);

void free_history_commands();

int parse_escape_sequence(char *input, int i);
int up_arrow(char *input, int i);
int down_arrow(char *input, int i);
int backspace(char *input, int i);

struct termios orig_termios;
bool is_interactive_global;

volatile __sig_atomic_t sigchld_received = 0;
void sigchld_handler(int sig);

int command_history_capacity = 16;
int command_counter = 0;
char **command_history;
int history_position = 0;
int history_append_position = 0;


int main() {

    char input[MAX_SIZE];
    char ch;
 
    command_history = malloc(command_history_capacity * sizeof(char *));

    signal(SIGCHLD, sigchld_handler);

    bool is_interactive = isatty(STDIN_FILENO); // is fd refering to terminal (tty) or something else (pipe |)
    is_interactive_global = is_interactive;

    struct termios raw;
    if (is_interactive)
    {
        tcgetattr(STDIN_FILENO, &orig_termios);
        raw = orig_termios;              // Copy for returing terminal to canonical mode
        raw.c_lflag &= ~(ECHO | ICANON); // Disable Canonical Mode
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        atexit(restore_terminal);
    }
    atexit(free_history_commands);
    atexit(write_history_on_exit);

    //reading history from histfile env var
    read_history_on_start();

    setbuf(stdout, NULL);
    while (1)
    {

        print_job_status("Done", 4);
        remove_done_jobs();

        printf("$ ");

        if (is_interactive)
        {
            int i = 0;
            ch = '\0';
            int tab_counter = 0;

            while (ch != '\n')
            {
                read(STDIN_FILENO, &ch, 1);

                if(sigchld_received)
                {
                    sigchld_received = 0;
                    mark_job_as_done();
                }

                if (ch == 9)    // tab
                {
                    tab_counter++;
                    i = handle_tab_completion(input, i, &tab_counter);
                   
                    continue;
                }
                else if (ch == 27)
                { // Start of esc sequence, cant move with arrows freely trough terminal
                    
                    i = parse_escape_sequence(input, i);
                    continue;
                }
                else if (ch == 127)
                { // backspace
                    if (i > 0)
                    {
                        i = backspace(input, i);
                        tab_counter = 0;
                    }
                }
                else
                {
                    if (i < MAX_SIZE - 1)
                    {
                        input[i++] = ch;
                        input[i] = '\0';
                        printf("%c", ch);

                        tab_counter = 0;
                    }
                }
                // printf("%d\n", ch);
            }
        }
        else
        {
            if (fgets(input, MAX_SIZE - 1, stdin) == NULL)
            {
                break; // EOF or error
            }

            if(sigchld_received)
            {
                sigchld_received = 0;
                mark_job_as_done();
            }
        }
        input[strcspn(input, "\n")] = '\0';

        //append to history
        if (command_counter >= command_history_capacity)
        {
            command_history_capacity *= 2;
            command_history = realloc(command_history, command_history_capacity * sizeof(char *));
        }
        command_history[command_counter++] = strdup(input);
        history_position = command_counter;

        // if input is blank or only spaces
        bool only_white_spaces = true;
        for (int i = 0; input[i] != '\0'; i++)
        {
            if (!isspace((unsigned char)input[i]))
            {
                only_white_spaces = false;
                break;
            }
        }
        if (only_white_spaces)
        {
            continue;
        }

        //parsing input
        bool is_background;
        Pipeline pipeline = parse_input(input, &is_background);

        //execute if pipes exist
        if(pipeline.num_of_commands > 1)
        {
            if(!execute_pipe(pipeline, is_background))
            {
                printf("%s: command not found\n", pipeline.command[0].args[0]);
            }
            free_commands(pipeline);
            continue;
        }

        int target_fd = 1;
        int fd = check_output_redirect(pipeline.command[0].args, &target_fd);

        if (pipeline.command[0].args[0] == NULL)
        {
            if (fd != -1)
                close(fd);
            free_commands(pipeline);
            continue;
        }

        if (run_builtin(pipeline.command[0].args, fd, target_fd))
        {
            free_commands(pipeline);
            continue;
        }
        else if (run_program(pipeline.command[0].args, fd, target_fd, is_background))
        {
            free_commands(pipeline);
            continue;
        }

        printf("%s: command not found\n", pipeline.command[0].args[0]);

        if (fd != -1) 
        {
            close(fd);   // if neither builtin nor program func closes it
        }
        free_commands(pipeline); // freeing the memory from func parse_input
    }

    return 0;
}


void free_history_commands()
{
    for(int i = 0; i < command_counter; i++)
    {
        free(command_history[i]);
    }
    free(command_history);
}

void restore_terminal(void)
{
    if (is_interactive_global)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

void sigchld_handler(int sig)
{
    (void)sig;
    sigchld_received = 1;
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