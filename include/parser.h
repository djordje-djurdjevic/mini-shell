#ifndef PARSER_H
#define PARSER_H

Pipeline parse_input(char *input, bool *is_background);
void free_commands(Pipeline pipeline);
int parse_escape_sequence(char *input, int i);
int up_arrow(char *input, int i);
int down_arrow(char *input, int i);
int backspace(char *input, int i);

#endif