#ifndef BUILTINS_H
#define BUILTINS_H

#include "common.h"

bool echo(char **args);
bool run_builtin(char **args, int fd, int target_fd);
bool type(char *input);
bool pwd(void);
bool cd(char *input);
bool complete(char **args);
bool history(char **args);
bool history_r_flag_helper(char **args);

bool background_jobs();
void remove_done_jobs();
void print_job_status(char *compare_for_output, int len_cmp_for_output);

extern const char *BUILTINS[];
extern const int BUILTINS_COUNT;

extern char registered_commands[][MAX_SIZE];
extern char registered_paths[][MAX_SIZE];
extern int registered_count;

#endif