#ifndef COMMON_H
#define COMMON_H

#include <termios.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define MAX_MATCHES 64
#define MAX_JOBS 32
#define STATUS_LEN 25

extern struct termios orig_termios;
extern bool is_interactive_global;

typedef struct {
    int job_number;
    pid_t pid;
    char *command;
    char status[STATUS_LEN];
} Job;

extern Job jobs[MAX_JOBS];
extern int job_count;
extern int next_job_number;

#endif