#ifndef EXECUTOR_H
#define EXECUTOR_H

bool run_program(char **args, int fd, int target_fd, bool is_background);
void mark_job_as_done();

#endif