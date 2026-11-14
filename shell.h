//
// Created by Lucas on 11/11/2026.
//
#include <stdio.h>
#include <stdlib.h>
#include "utility.h"
#include "filesystem.h"
#include <ctype.h>

#ifndef LINUXFILEEMULATOR_SHELL_H
#define LINUXFILEEMULATOR_SHELL_H
typedef int (*CommandFunc)(int, char**);

typedef struct {
    char* name;
    int arg_num;
    CommandFunc func;
} Command;

void open_shell(int new_fs);
int handle_input(char *input);
int words_in_input(const char *input);
char** parse_args(char *input, int word_count);
void free_args(char** args, int words) ;
int run_command(char **args, int arg_num);
int format_path(char** path);
#endif //LINUXFILEEMULATOR_SHELL_H