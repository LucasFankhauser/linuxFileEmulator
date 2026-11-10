//
// Created by Lucas on 11/9/2026.
//
#include <stdio.h>
#include <unistd.h>

#ifndef LINUXFILEEMULATOR_FS_UTILITY_H
#define LINUXFILEEMULATOR_FS_UTILITY_H

void fs_write_num(int fs, int offset, int space, int num );
void fs_write(int fs, int offset, int space, char* line);
int fs_read_num(int fs, int offset, int space);
char* fs_read(int fs, int offset, int space);

#endif //LINUXFILEEMULATOR_FS_UTILITY_H