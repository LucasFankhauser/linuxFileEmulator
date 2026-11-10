//
// Created by lucas on 11/8/2026.
//

#include "filesystem.h"

#include "fs_utility.h"

int init() {
    printf("Initiating Filesystem\n");


    Config* config = create_config();

    if (config == NULL) {
        return -1;
    }

    if (check_config(config) == -1) {
        free(config);
        return -1;
    }

    int file = open( "filesystem", O_RDWR | O_APPEND, 0700 );

    if (file == -1) {
        printf("No filesystem detected creating new one\n");
        create_fs(config);
        free(config);
        return 0;
    }


    if (config->RESET == 1) {
        parse_set(config->CONFIG_FILE, "0", "RESET");
        config->RESET = 0;

        printf("Resetting current filesystem\n");
        create_fs(config);
    } else {
        printf("Opening current filesystem\n");
        open_fs();
    }

    free(config);
    return 0;
}

int open_fs() {
    return 0;
}

int create_fs(Config* config) {

    int fs = open( "filesystem", O_RDWR | O_CREAT, 0700 );

    if (fs == -1) {
        printf("Failed to create file for filesystem\n");
        return -1;
    }

    char buffer[config->DISK_SIZE];
    memset(buffer, '_', config->DISK_SIZE);

    write(fs, buffer, config->DISK_SIZE);

    int inode_size = 50;
    write_super_block(fs, config);
    int pos = 40;
    for (int i = 0; i < config->GROUPS; i++) {

        char inode_bitmask[config->INODE_NUM]; //CREATE INODE BITMASK
        memset(inode_bitmask, '0', config->INODE_NUM);
        fs_write(fs, pos, config->INODE_NUM, inode_bitmask);
        pos += config->INODE_NUM;

        char data_block_bitmask[config->BLOCK_NUM]; //CREATE DATABLOCK BITMASK
        memset(data_block_bitmask, '0', config->BLOCK_NUM);
        fs_write(fs, pos, config->BLOCK_NUM, data_block_bitmask);
        pos += config->BLOCK_NUM;

        for (int j = 0; j < config->INODE_NUM; j++) { //CREATE INODES
            char inode[inode_size];                                     //CREATE INODE
            memset(inode, '?', inode_size);
            fs_write(fs, pos, inode_size, inode);
            pos += inode_size;
        }

        for (int j = 0; j < config->BLOCK_NUM; j++) { //CREATE DATABLOCKS
            char block[config->BLOCK_SIZE];                                     //CREATE DATA BLOCK
            memset(block, '?', inode_size);
            fs_write(fs, pos, config->BLOCK_SIZE, block);
            pos += config->BLOCK_SIZE;
        }

    }
    printf("%d\n", pos);
    close(fs);
}
void write_super_block(int fs, Config* config) {
    fs_write_num(fs, 0, 8,config->DISK_SIZE);
    fs_write_num(fs, 8, 8,config->GROUPS);
    fs_write_num(fs, 16, 8,config->BLOCK_SIZE);
    fs_write_num(fs, 24, 8,config->BLOCK_NUM);
    fs_write_num(fs, 32, 8,config->INODE_NUM);
}

Config* read_super_block(int fs) {
    Config* superblock = malloc(sizeof(Config));
    if (superblock == NULL) {
        return NULL;
    }
    superblock-> DISK_SIZE = fs_read_num(fs, 0, 8);
    superblock-> GROUPS = fs_read_num(fs, 8, 8);
    superblock-> BLOCK_SIZE = fs_read_num(fs, 16, 8);
    superblock-> BLOCK_NUM = fs_read_num(fs, 24, 8);
    superblock-> INODE_NUM = fs_read_num(fs, 32, 8);
    return superblock;
}

void print_super_block(Config* superblock) {
    printf("Disk size: %d\n", superblock-> DISK_SIZE);
    printf("Groups: %d\n", superblock-> GROUPS);
    printf("Block Size: %d\n", superblock-> BLOCK_SIZE);
    printf("Block Num: %d\n", superblock-> BLOCK_NUM);
    printf("INode Num: %d\n", superblock-> INODE_NUM);
}

Config* create_config() {

    int config_file = open( "config", O_RDWR, 0700 );
    if (config_file == -1) {
        printf("Failed to open config file\n");
        return NULL;
    }

    Config* config = malloc(sizeof(Config));

    if (config == NULL) {
        printf("Failed to allocate memory for config\n");
        free(config);
        return NULL;
    }

    char output[1024];
    int failed_parse = 0;
    if (parse_get(config_file, output, "DISK_SIZE") == -1) {failed_parse = -1;};
    config->DISK_SIZE = atoi(output);
    if (parse_get(config_file, output, "BLOCK_NUM") == -1) {failed_parse = -1;};
    config->BLOCK_NUM = atoi(output);
    if (parse_get(config_file, output, "BLOCK_SIZE") == -1) {failed_parse = -1;};
    config->BLOCK_SIZE = atoi(output);
    if (parse_get(config_file, output, "GROUPS") == -1) {failed_parse = -1;};
    config->GROUPS = atoi(output);
    if (parse_get(config_file, output, "INODE_NUM") == -1) {failed_parse = -1;};
    config->INODE_NUM = atoi(output);
    if (parse_get(config_file, output, "RESET") == -1) {failed_parse = -1;};
    config->RESET = atoi(output);
    config->CONFIG_FILE = config_file;
    if (failed_parse == -1) {
        printf("Failed to parse config file\n");
        free(config);
        return NULL;
    }

    return config;
}

int check_config(Config* config) {

    if (config-> DISK_SIZE <= 0) {
        printf("Disk size must be greater than zero\n");
        return -1;
    }
    if (config-> BLOCK_NUM <= 0) {
        printf("Block size must be greater than zero\n");
        return -1;
    }
    if (config-> BLOCK_SIZE <= 0) {
        printf("Block size must be greater than zero\n");
        return -1;
    }
    if (config-> GROUPS <= 0) {
        printf("Groups must be greater than zero\n");
        return -1;
    }
    if (config-> INODE_NUM <= 0) {
        printf("Inode num must be greater than zero\n");
        return -1;
    }

    int super_block_size = 40;
    int inode_size = 50;

    int needed_space = 0;


    needed_space += super_block_size; //space for superblock

    int space_per_group = 0;
    space_per_group += config->INODE_NUM; //Space for inode bitmap
    space_per_group += config->BLOCK_NUM; //space for datablock bitmap
    space_per_group += config->INODE_NUM * inode_size; //space for inodes
    space_per_group += config->BLOCK_NUM * config->BLOCK_SIZE; //space for blocks

    needed_space += space_per_group * config->GROUPS;

    if (needed_space > config->DISK_SIZE || needed_space < 0) {
        printf("Config does not fit in disk size. DISK_SIZE: %d CONFIG SIZE: %d\n", config->DISK_SIZE, needed_space);
        return -1;
    }
    printf("Valid Config. DISK_SIZE: %d CONFIG SIZE: %d\n", config->DISK_SIZE, needed_space);
    return 0;
}
