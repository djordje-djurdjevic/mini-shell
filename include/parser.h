#ifndef PARSER_H
#define PARSER_H

char **parse_input(char *input, bool *is_background);
void free_args(char **args);

#endif