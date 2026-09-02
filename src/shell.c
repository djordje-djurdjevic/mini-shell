#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "common.h"
#include "builtins.h"
#include "completion.h"
#include "executor.h"
#include "parser.h"
#include "redirect.h"

void restore_terminal(void)
{
    if (is_interactive_global)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

struct termios orig_termios;
bool is_interactive_global;

volatile __sig_atomic_t sigchld_received = 0;

void sigchld_handler(int sig)
{
    sigchld_received = 1;
}
signal(SIGCHLD, sigchld_handler);

int main() {

    char input[MAX_SIZE];
    char ch;

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

    setbuf(stdout, NULL);
    while (1)
    {

        if(sigchld_received)
        {
            sigchld_received = 0;
            cleanup_finished_jobs();
        }

        printf("$ ");

        if (is_interactive)
        {
            int i = 0;
            ch = '\0';
            int tab_counter = 0;

            while (ch != '\n')
            {
                read(STDIN_FILENO, &ch, 1);

                if (ch == 9)    // tab
                {
                    tab_counter++;
                    i = handle_tab_completion(input, i, &tab_counter);
                   
                    continue;
                }
                else if (ch == 27)
                { // Start of esc sequence, cant move with arrows freely trough terminal
                    char seq[2];
                    read(STDIN_FILENO, &seq[0], 1);
                    read(STDIN_FILENO, &seq[1], 1);
                    // ignore sequence do nothing
                    continue;
                }
                else if (ch == 127)
                { // backspace
                    if (i > 0)
                    {
                        i--;
                        input[i] = '\0';
                        printf("\b \b");
                        fflush(stdout);

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
        }
        input[strcspn(input, "\n")] = '\0';

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

        bool is_background;
        char **args = parse_input(input, &is_background);
        int target_fd = 1;
        int fd = check_output_redirect(args, &target_fd);

        if (args[0] == NULL)
        {
            if (fd != -1)
                close(fd);
            free_args(args);
            continue;
        }

        if (run_builtin(args, fd, target_fd))
        {
            free_args(args);
            continue;
        }
        else if (run_program(args, fd, target_fd, is_background))
        {
            free_args(args);
            continue;
        }

        printf("%s: command not found\n", input);

        if (fd != -1) 
        {
            close(fd);   // if neither builtin nor program func closes it
        }
        free_args(args); // freeing the memory from func parse_input
    }

    return 0;
}