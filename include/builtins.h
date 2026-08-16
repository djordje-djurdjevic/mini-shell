#ifndef BUILTINS_H
#define BUILTINS_H

bool echo(char **args);
bool run_builtin(char **args, int fd, int target_fd);
bool type(char *input);
bool pwd(void);
bool cd(char *input);

#endif