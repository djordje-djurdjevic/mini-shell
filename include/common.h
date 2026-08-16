#ifndef COMMON_H
#define COMMON_H

#include <termios.h>
#include <stdbool.h>

#define MAX_SIZE 1024
#define MAX_MATCHES 64

extern struct termios orig_termios;
extern bool is_interactive_global;

#endif