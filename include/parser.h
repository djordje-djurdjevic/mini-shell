#ifndef PARSER_H
#define PARSER_H

Pipeline parse_input(char *input, bool *is_background);
void free_commands(Pipeline pipeline);
void variable_swap_helper(bool *variable_swap, char arg[]);

#endif