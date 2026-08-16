#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>


#include "common.h"
#include "completion.h"

int handle_tab_completion(char *input, int i, int tab_counter)
{
    // printf("[DEBUG input='%s' i=%d]", input, i);
    if (i == 0) {
        printf("\a"); // beep
        return i;
    }

    char matches[MAX_MATCHES][MAX_SIZE];
    int match_count = 0;

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

    char *builtins[] = {"echo", "exit", "type", "pwd", "cd"};
    int length = sizeof(builtins) / sizeof(builtins[0]);

    for (int j = 0; j < length; j++)
    {
        if (strncmp(builtins[j], input, i) == 0)
        {
            strcpy(matches[*match_count], builtins[j]);
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

                        if (!already_seen && *match_count < MAX_MATCHES)
                        {
                            strcpy(matches[*match_count], entry->d_name);
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

int resolve_completion(char *input, int start_idx, int prefix_len, char matches[][MAX_SIZE], int match_count, int tab_counter)
{

    if (match_count == 0)
    {
        printf("\a"); // beep
        return start_idx + prefix_len;
    }
    else if (match_count == 1)
    {
        input[start_idx] = '\0';
        strcat(input, matches[0]);
        strcat(input, " ");
        printf("%s ", matches[0] + prefix_len);
        return strlen(input);
    }
    else
    {

        if (tab_counter % 2 == 0)
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
            int new_i = longest_common_prefix(matches, match_count);
            input[start_idx] = '\0';
            strcat(input, matches[0]);
            printf("%s", matches[0] + prefix_len);

            return start_idx + new_i;
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

    strcpy(matches[0], lcp);
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