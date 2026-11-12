//
// Created by Lucas on 11/11/2026.
//


#include "shell.h"

#include <ctype.h>


const int MAX_INPUT_LENGTH = 1024;

char current_dir[1024];

void open_shell(int fs) {
    printf("Shell initialized\n");
    int running = 1;


    while (running == 1) {
        printf("filesystem:~$ ");
        char input_buffer[MAX_INPUT_LENGTH];

        if (fgets(input_buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
            continue;
        }
        int status = handle_input(fs, input_buffer);
        if (status == -1) {
            running = 0;
        }

    }
    printf("Shell closed\n");
}


int handle_input(int fs, char *input) {
    int words = words_in_input(input);
    if (words == 0) {
        printf("Empty input\n");
        return 0;
    }
    char** args = parse_args(input, words);
    int status = run_command(fs, args, words);
    free_args(args, words);
    return status;
}

int run_command(int fs, char **args, int arg_num) {
    char* command = args[0];
    if (command == NULL) {
        printf("command is NULL\n");
        return 0;
    }

    if (str_match(command, "exit") == 0) {
        if (arg_num != 1) {
            printf("exit requires 0 arguments\n");
            return 0;
        }
        return -1;

    } else if (str_match(command, "cd") == 0) {
        if (arg_num != 2) {
            printf("cd requires 1 arguments\n");
            return 0;
        }
        printf("cd command\n");
        return 1;
    }
    printf("Unrecognized command: %s\n", command);
    return 0;
}

int words_in_input(const char *input) {
    int word_count = 0;
    int hit_space = 1;
    for (int i = 0; input[i] != '\0' || i == MAX_INPUT_LENGTH; i++) {
        if (input[i] == ' ' || input[i] == '\n' || input[i] == '\t') {
            hit_space = 1;
        } else if (hit_space == 1) {
            hit_space = 0;
            word_count++;
        }
    }
    return word_count;
}


void free_args(char** args, int words) {
    for (int i = 0; i < words; i++) {
        if (args[i] != NULL) {
            free(args[i]);
        }
    }
    free(args);
}

char** parse_args(char *input, int word_count) {
    char** args = calloc(word_count + 1, sizeof(char*));
    if (args == NULL) {
        return NULL;
    }

    int word_index = 0;
    int word_start = -1;

    for (int i = 0; i <= MAX_INPUT_LENGTH && input[i] != '\0'; i++) {

        if (!isspace(input[i])) {
            if (word_start == -1) {
                word_start = i;
            }
        }

        if (word_start != -1 && is_space(input[i])) {

            if (word_index >= word_count) {
                free_args(args, word_count + 1);
                return NULL;
            }

            int length = i - word_start;

            args[word_index] = malloc(length + 1);

            if (args[word_index] == NULL) {
                free_args(args, word_count + 1);
                return NULL;
            }
            for (int j = 0; j < length; j++) {
                args[word_index][j] = input[word_start + j];
            }

            args[word_index][length] = '\0';

            word_index++;
            word_start = -1;
             }
    }

    return args;
}