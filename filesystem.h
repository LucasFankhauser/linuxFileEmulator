//
// Created by lucas on 11/8/2026.
//


#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

typedef struct {
    int DISK_SIZE;
    int GROUPS;
    int BLOCK_SIZE;
    int BLOCK_NUM;
    int INODE_NUM;
    int RESET;
    int CONFIG_FILE;
} Config;

Config* create_config();
int init();
int open_fs();
int create_fs();
int check_config(Config* config);
void write_super_block(int fs, Config* config);
Config* read_super_block(int fs);
void print_super_block(Config* superblock);

#endif //FILESYSTEM_H
