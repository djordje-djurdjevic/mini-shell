#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "common.h"
#include "completion.h"
#include "builtins.h"
#include "parser.h"


int handle_tab_completion(char *input, int i, int *tab_counter)
{
    //printf("[DEBUG input='%s' i=%d]", input, i);
    if (i == 0) {
        printf("\a"); // beep
        return i;
    }

    char matches[MAX_MATCHES][MAX_SIZE];
    matches[0][0] = '\0';
    int match_count = 0;

    if(check_completer(input, matches)) 
    {   
        //printf("DEBUG: Entered check completer");
        //printf("\nDEBUG BEFORE: input='%s' i=%d\n", input, i);
        // printf("DEBUG MATCH: '%s'\n", matches[0]);
        if (strcmp(matches[0], "") == 0) {
            printf("\a"); // beep
            return i;
        }

        //printf("DEBUG: %s", matches[0]);
        return resolve_completion(input, i, 0, matches, 1, tab_counter);
    }

    if(i == 0 || is_first_token(input, i)) {

        check_builtin_matches(input, i, matches, &match_count);
        if (match_count == 1) {
            return resolve_completion(input, 0, i, matches, 1, tab_counter); //edge case starting with spaces
        } else {

            check_path_matches(input, i, matches, &match_count, getenv("PATH"), true);
            return resolve_completion(input, 0, i, matches, match_count, tab_counter);
        }
    } else {
        //printf("DEBUG: enter else branch");
        int prefix_i = i;                       // length of last arg (prefix)
        char *last_arg = get_last_arg(input, &prefix_i);
        
        char *last_slash = strrchr(last_arg, '/');

        if (last_slash != NULL ) { //nested file
            int dirlen = last_slash - last_arg + 1;

            char dir_part[MAX_SIZE];
            strncpy(dir_part, last_arg, dirlen);
            dir_part[dirlen] = '\0'; 

            char *file_prefix = last_arg + dirlen;
            int real_start_file = strlen(file_prefix);

            char full_dir[MAX_SIZE];
            char *cwd = getcwd(NULL, 0);
            strcpy(full_dir, cwd);
            strcat(full_dir, "/");
            strcat(full_dir, dir_part);

            check_path_matches(file_prefix, real_start_file, matches, &match_count, full_dir, false);

            free(last_arg);
            free(cwd);

            int real_start = i - prefix_i;  // starting pos in buffer

            for (int k = 0; k < match_count; k++) {
                char temp_matches[MAX_SIZE];
                strcpy(temp_matches, dir_part);
                strcat(temp_matches, matches[k]);
                strcpy(matches[k], temp_matches);
            }


            return resolve_completion(input, real_start, prefix_i, matches, match_count, tab_counter);

        } else { //no nested file
            //printf("[DEBUG last_arg='%s' i_copy=%d]\n", last_arg, i_copy);
            char *cwd = getcwd(NULL, 0);        
            check_path_matches(last_arg, prefix_i, matches, &match_count, cwd, false);
            
            //printf("[DEBUG match_count='%d' ]", match_count);
            int real_start = i - prefix_i;          // starting pos in buffer

            free(last_arg);
            free(cwd);

            return resolve_completion(input, real_start, prefix_i, matches, match_count, tab_counter);
        }
    }
    // printf("[DEBUG match_count='%d' ]", match_count);
}

void check_builtin_matches(char *input, int i, char matches[][MAX_SIZE], int *match_count)
{
    for (int j = 0; j < BUILTINS_COUNT; j++)
    {
        if (strncmp(input, BUILTINS[j], i) == 0)
        {
            strcpy(matches[*match_count], BUILTINS[j]);
            (*match_count)++;
        }
    }
}

void check_path_matches(char *input, int i, char matches[][MAX_SIZE], int *match_count, char *dir_path, bool require_exec)
{

    char path_cpy[MAX_SIZE];
    strncpy(path_cpy, dir_path, sizeof(path_cpy) - 1);
    path_cpy[sizeof(path_cpy) - 1] = '\0';

    char *all_paths = strtok(path_cpy, ":");
    char full_path[MAX_SIZE];

    while (all_paths != NULL)
    {
        DIR *dir = opendir(all_paths);
        if (dir != NULL)
        {
            struct dirent *entry;

            while ((entry = readdir(dir)) != NULL)
            {

                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                if (strncmp(entry->d_name, input, i) == 0)
                {

                    snprintf(full_path, sizeof(full_path), "%s/%s", all_paths, entry->d_name);

                    bool ok = require_exec
                    ? (access(full_path, F_OK) == 0 && access(full_path, X_OK) == 0)
                    : (access(full_path, F_OK) == 0);

                    if (ok)
                    {

                        bool already_seen = false;
                        for (int k = 0; k < *match_count; k++)
                        {
                            if (strcmp(matches[k], entry->d_name) == 0)
                            {
                                //printf("[DEDUP HIT: %s already at index %d]", entry->d_name, k);
                                already_seen = true;
                                break;
                            }
                        }


                        struct stat st;

                        if (!already_seen && *match_count < MAX_MATCHES)
                        {
                            if(stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) //is dir?
                            {
                                strcpy(matches[*match_count], entry->d_name);
                                strcat(matches[*match_count], "/");
                            } 
                            else 
                            {
                                strcpy(matches[*match_count], entry->d_name);
                            } 
                            (*match_count)++;
                        }
                    }
                }
            }

            closedir(dir);
        }

        all_paths = strtok(NULL, ":");
    }
}

int resolve_completion(char *input, int start_idx, int prefix_len, char matches[][MAX_SIZE], int match_count, int *tab_counter)
{

    if (match_count == 0)
    {
        printf("\a"); // beep
        return start_idx + prefix_len;
    }
    else if (match_count == 1)
    {   
        strcpy(input + start_idx, matches[0]);

        if(matches[0][strlen(matches[0]) - 1] == '/') //directory
        {
            printf("%s", matches[0] + prefix_len);
        }
        else
        {
            strcat(input, " ");
            printf("%s ", matches[0] + prefix_len);
        }

        // printf("\nDEBUG input='%s' len=%zu return=%zu\n",
        // input,
        // strlen(input),
        // strlen(input));

        (*tab_counter) = 0; 
        return strlen(input);
    }
    else
    {   
        int lcp_len = longest_common_prefix(matches, match_count);
        if (lcp_len != prefix_len) // lcp exists 
        {
            strncat(input, matches[0] + prefix_len, lcp_len - prefix_len);
            printf("%.*s", lcp_len - prefix_len, matches[0] + prefix_len);

            (*tab_counter) = 0;
            return start_idx + lcp_len;
        }
         

        if ( (*tab_counter) > 1)
        {
            printf("\n");
            for (int k = 0; k < match_count; k++)
            {
                printf("%s  ", matches[k]);
            }
            printf("\n$ %s", input);
        }
        else
        {
            printf("\a"); // beep
        }

        return start_idx + prefix_len;
    }
}

int longest_common_prefix(char matches[][MAX_SIZE], int match_count)
{
    char lcp[MAX_SIZE];
    strcpy(lcp, matches[0]); // start with first candidate

    for (int k = 1; k < match_count; k++)
    {

        int j = 0;
        while (lcp[j] != '\0' && matches[k][j] != '\0' && lcp[j] == matches[k][j])
        {
            j++;
        }

        lcp[j] = '\0';
    }

    //strcpy(matches[0], lcp);
    return strlen(lcp);
}

char *get_last_arg(const char *input, int *cursor_pos) {
    int  i = *cursor_pos;

    while(i > 0 && input[i- 1] != ' ') {
        i--;
    }

    int prefix_len = (*cursor_pos) - i; 
    *cursor_pos = prefix_len;  

    return strndup(input + i, prefix_len);
}

bool is_first_token(const char *input, int cursor_pos) {
    int i = cursor_pos;
    // move to the left until spacebar
    while (i > 0 && input[i - 1] != ' ') {
        i--;
    }
    // if non space char exists before i
    for (int j = 0; j < i; j++) {
        if (input[j] != ' ') {
            return false; // found something isnt the first word
        }
    }
    return true; //it is first token
}

bool check_completer(char *input, char matches[][MAX_SIZE]) {

    //printf("[DEBUG registered_count=%d]\n", registered_count);

    char **args = parse_input(input);
    int args_count = 0;
    while (args[args_count] != NULL) args_count++;

    for (int i = 0; i < registered_count; i++) 
    {
        //printf("[DEBUG comparing input='%s' vs registered='%s']\n", input, registered_commands[i]);
        if (strcmp(args[0], registered_commands[i]) == 0)
        {
            int fd[2];
            pipe(fd); 

            pid_t pid = fork();

            if (pid == -1) 
            {
                close(fd[0]);
                close(fd[1]);
                printf("fork error");
            }
            else if(pid == 0) 
            {
                setenv("COMP_LINE", input, 1);

                char comp_point_str[16];
                snprintf(comp_point_str, sizeof(comp_point_str), "%d", (int)strlen(input));
                setenv("COMP_POINT", comp_point_str, 1);


                char *prev_word    = (args_count >= 2) ? args[args_count - 2] : ""; //git remote set
                char *current_word = (args_count >= 1) ? args[args_count - 1] : "";

                char *completer_args[5];
                completer_args[0] = registered_paths[i]; //git remote set
                completer_args[1] = args[0];
                completer_args[2] = current_word;
                completer_args[3] = prev_word;
                completer_args[4] = NULL;

                dup2(fd[1], STDOUT_FILENO);
                execvp(completer_args[0], completer_args);
                //perror("execvp failed");   

                close(fd[0]);
                close(fd[1]);
                free(args);

                _exit(127);
            }
            else 
            {
                close(fd[1]);

                int status;
                wait(&status);

                if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
                {
                    //printf("DEBUG: FALSE");
                    free(args);
                    return false;
                }

                char buffer[MAX_SIZE];
                int bytes_read = read(fd[0], buffer, sizeof(buffer) - 1);
                //printf("[DEBUG bytes_read=%d buffer='%s']\n", bytes_read, buffer);
                if (bytes_read > 0) 
                {
                    buffer[bytes_read] = '\0';
                    //#pragma GCC diagnostic push
                    //#pragma GCC diagnostic ignored "-Wformat-truncation"
                    snprintf(matches[0], MAX_SIZE, "%s", buffer);
                    //#pragma GCC diagnostic pop
                }
                
                close(fd[0]);
                free(args);

                return true;
                //resolve_completion(NULL, 0, 0, matches, 1, 0);
            }
        }
    }

    return false;
}