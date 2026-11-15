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

//FILESYSTEM
Config* create_config();
int get_inode_null_value();
int get_file(int fs, int dir_node, char* fname);
int get_max_file_name();
int init();
int open_fs();
int create_fs(Config* config);
int check_config(const Config* config);
void write_super_block(int fs, Config* config);
Config* read_super_block(int fs);
void print_super_block(Config* superblock);
void create_dir(int fs, const char* path, char* dname);
int group_size(const Config* config);
void create_root_directory(int fs);
void create_file(int fs, const char* path, char* fname);
int delete_file_by_id(int fs, int id);
int delete_file(int fs, const char* path, char* fname);
int delete_directory(int fs, const char* path, char* dname, int safe);
int delete_directory_by_id(int fs, int did , int safe);

//INODES
int create_inode(int fs);

void set_inode_file_type(int fs, int id, char value);
char *get_inode_file_type(int fs, int id);
void set_inode_permissions(int fs, int id, char *value);
char *get_inode_permissions(int fs, int id);
void set_inode_owner_uid(int fs, int id, int value);
int get_inode_owner_uid(int fs, int id);
void set_inode_group_uid(int fs, int id, int value);
int get_inode_group_uid(int fs, int id);
void set_inode_file_size(int fs, int id, int value);
int get_inode_file_size(int fs, int id);
void set_inode_hard_links(int fs, int id, int value);
int get_inode_hard_links(int fs, int id);
void set_inode_inode_id(int fs, int id, int value);
int get_inode_inode_id(int fs, int id);
void set_inode_allocated_blocks(int fs, int id, int value);
int get_inode_allocated_blocks(int fs, int id);
void set_inode_array(int fs, int id, int pos, int value);
int get_inode_array(int fs, int id, int pos);

void set_inode_bitmap(int fs, int id, int value);
int inode_add_block(int fs, int id);
int write_to_inode(int fs, int inode, int offset, char* write_buffer, int write_len);
int get_inode_block_id(int fs, int inode, int block_num);
int read_from_inode(int fs, int inode, int offset, char* write_buffer, int read_len);
int write_num_to_inode(int fs, int inode, int offset, int value, int space);
int read_num_from_inode(int fs, int inode, int offset, int read_len);
int allocate_at_pos(int fs, int pos, int indirection, int id, int array_index);
int write_word_to_inode(int fs, int inode, int offset, char* write_buffer, int write_len);
int read_word_from_inode(int fs, int inode, int offset, char* read_buffer, int read_len);
int get_available_inode_id(int fs);
int get_inode_offset(int fs, int id);
void print_inode_stat(int fs, int inode);
int remove_inode_contents(int fs, int id);

//BLOCKS
int allocate_block(int fs);
int allocate_block_in_array(int fs, int pos, int indirection, int block_id, int id);
int format_block_as_indirect(int fs, int block_id);
int get_block_id_at_pos(int fs, int pos, int indirection, int block_id);
int get_block_offset(int fs, int id);
int set_block_bitmap(int fs, int id, int value);
int get_available_block_id(int fs);
int free_block(int fs, int bid, int indirection);
int remove_blocks(int fs, int bid, int num, int indirection);

//PATH
int get_path_name(int index, const char* path, char* file_name);
int follow_path(int fs, const char* path);
int get_dir_path(int fs, int inode, char** path);
#endif //FILESYSTEM_H


