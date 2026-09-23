#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"

bool history(char **args);
bool history_r_flag_helper(char **args);
bool history_w_flag_helper(char **args);
bool history_a_flag_helper(char **args);

void write_history_on_exit();
void read_history_on_start();

#endif