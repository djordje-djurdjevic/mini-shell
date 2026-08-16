#ifndef REDIRECT_H
#define REDIRECT_H

int check_output_redirect(char **args, int *target_fd);
void restore_std(int fd, int saved_std, int target_fd);

#endif