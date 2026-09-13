#ifndef COMMON_H
#define COMMON_H

#include <termios.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define MAX_MATCHES 64
#define MAX_JOBS 32
#define STATUS_LEN 25
#define MAX_PIPELINE 16


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


typedef struct {
    char **args;
} Command;

typedef struct {
    Command command[MAX_PIPELINE];
    int num_of_commands;
} Pipeline;

extern int command_history_capacity;
extern int command_counter;
extern char **command_history;

#endif