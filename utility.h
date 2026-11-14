//
// Created by Lucas on 11/9/2026.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef LINUXFILEEMULATOR_UTILITY_H
#define LINUXFILEEMULATOR_UTILITY_H


int parse_get(int file, char* buffer, char* attr);
int str_match(const char* str1, const char* str2);
long get_pos(int file, char* attr);
int parse_set(int file, char* value, char* attr);
int get_len(char* char_array, int max);
int is_space(char c);

#endif //LINUXFILEEMULATOR_UTILITY_H