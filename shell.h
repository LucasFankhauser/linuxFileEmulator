//
// Created by Lucas on 11/11/2026.
//
#include <stdio.h>
#include <stdlib.h>
#include "utility.h"
#ifndef LINUXFILEEMULATOR_SHELL_H
#define LINUXFILEEMULATOR_SHELL_H

void open_shell(int fs);
int handle_input(int fs, char *input);
int words_in_input(const char *input);
char** parse_args(char *input, int word_count);
void free_args(char** args, int words) ;
int run_command(int fs, char **args, int arg_num);
#endif //LINUXFILEEMULATOR_SHELL_H