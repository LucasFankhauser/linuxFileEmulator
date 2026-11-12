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
int check_config(const Config* config);
void write_super_block(int fs, Config* config);
Config* read_super_block(int fs);
int get_avaible_inode_id(int fs);
void print_super_block(Config* superblock);
void create_dir(int fs, char* path, char* dname);
int group_size(const Config* config);
int get_available_block_id(int fs);
int get_available_inode_id(int fs);
int get_inode_offset(int fs, int id);
int get_block_offset(int fs, int id);
int set_block_bitmap(int fs, int id, int value);
void set_inode_bitmap(int fs, int id, int value);
void set_inode_file_type(int fs, int id, char value);
void set_inode_permissions(int fs, int id, char* value);
void set_inode_owner_uid(int fs, int id, int value);
void set_inode_group_uid(int fs, int id, int value);
void set_inode_file_size(int fs, int id, int value);
void set_inode_hard_links(int fs, int id, int value);
void set_inode_inode_id(int fs, int id, int value);
void set_inode_array(int fs, int id, int pos, int value);
int create_inode(int fs);
int get_inode_file_size(int fs, int id);
int get_inode_array(int fs, int id, int pos);
void set_inode_allocated_blocks(int fs, int id, int value);
int get_inode_allocated_blocks(int fs, int id);
int allocate_block(int fs);
int allocate_block_in_array(int fs, int pos, int indirection, int block_id, int id);
int inode_add_block(int fs, int id);
int format_block_as_indirect(int fs, int block_id);
int allocate_at_pos(int fs, int pos, int indirection, int id, int array_index);
int write_to_inode(int fs, int inode, int offset, char* write_buffer, int write_len);
int get_inode_block_id(int fs, int inode, int block_num);
int get_block_id_at_pos(int fs, int pos, int indirection, int block_id);
int read_from_inode(int fs, int inode, int offset, char* write_buffer, int read_len);
#endif //FILESYSTEM_H


