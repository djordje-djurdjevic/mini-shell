#ifndef COMPLETION_H
#define COMPLETION_H

#include "common.h"

int handle_tab_completion(char *input, int i, int tab_counter);
void check_builtin_matches(char *input, int i, char matches[][MAX_SIZE], int *match_count);
void check_path_matches(char *input, int i, char matches[][MAX_SIZE], int *match_count, char *dir_path, bool require_exec);
int resolve_completion(char *input, int start_idx, int prefix_len, char matches[][MAX_SIZE], int match_count, int tab_counter);
int longest_common_prefix(char matches[][MAX_SIZE], int match_count);

char *get_file_completion_prefix(const char *input, int *cursor_pos);
bool is_first_token(const char *input, int cursor_pos);

#endif