//
// Created by lucas on 11/8/2026.
//

#include "filesystem.h"

#include "fs_utility.h"

const int SUPER_BLOCK_SIZE = 40;
const int INODE_SIZE = 178;
const int INODE_ARRAY_SIZE = 15;
const int INODE_NULL_VALUE = 99999990;

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

    int file = open( "filesystem", O_RDWR, 0700 );

    if (file == -1) {
        printf("No filesystem detected creating new one\n");
        create_fs(config);
        free(config);
        return 0;
    }

    int fs;
    if (config->RESET == 1) {
        parse_set(config->CONFIG_FILE, "0", "RESET");
        config->RESET = 0;

        printf("Resetting current filesystem\n");
        if (create_fs(config)) {
            fs = open_fs();
        } else {
            fs = -1;
        }
    } else {
        printf("Opening current filesystem\n");
        fs = open_fs();
    }

    free(config);
    return fs;
}


int open_fs() {
    int fs = open( "filesystem", O_RDWR | O_CREAT, 0700 );

    if (fs == -1) {
        printf("Failed to open file for filesystem\n");
        return -1;
    }
    create_dir(fs, "", "~");
    return fs;
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

    write_super_block(fs, config);
    int pos = SUPER_BLOCK_SIZE;
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
            char inode[INODE_SIZE];                                     //CREATE INODE
            memset(inode, '?', INODE_SIZE);
            fs_write(fs, pos, INODE_SIZE, inode);
            pos += INODE_SIZE;
        }

        for (int j = 0; j < config->BLOCK_NUM; j++) { //CREATE DATABLOCKS
            char block[config->BLOCK_SIZE];                                     //CREATE DATA BLOCK
            memset(block, '?', config->BLOCK_SIZE);
            fs_write(fs, pos, config->BLOCK_SIZE, block);
            pos += config->BLOCK_SIZE;
        }

    }

    create_dir(fs, "", "~");

    write_to_inode(fs, 0, 0, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", -1);
    char read_buffer[209];
    read_from_inode(fs, 0, 0,read_buffer, 208);
    read_buffer[208] = '\0';
    printf("Read buffer: %s\n", read_buffer);
    close(fs);
    return 0;
}

int write_to_inode(int fs, int inode, int offset, char* write_buffer, int write_len) {
    if (write_len == -1) {
        write_len = get_len(write_buffer, -1);
    }
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }

    int end_offset = offset + write_len;
    int allocated_space = get_inode_allocated_blocks(fs, inode) * superblock->BLOCK_SIZE;
    //create space for write
    while (end_offset > allocated_space) {
        inode_add_block(fs, inode);
        allocated_space += superblock->BLOCK_SIZE;
    }
    if (end_offset > get_inode_file_size(fs, inode)) {
        set_inode_file_size(fs, inode, end_offset);
    }

    int block_num = offset / superblock->BLOCK_SIZE;
    int block_offset = offset - (block_num * superblock->BLOCK_SIZE);
    int write_offset = 0;

    while (write_len - write_offset > 0) {
        int block_id = get_inode_block_id(fs, inode, block_num);
        int block_pos = get_block_offset(fs, block_id);
        char write_char = write_buffer[write_offset];
        fs_write(fs, block_pos + block_offset, 1, &write_char);
        block_offset += 1;
        write_offset += 1;
        int remaining_space = superblock->BLOCK_SIZE - block_offset;

        if (remaining_space <= 0) {
            block_num += 1;
            block_offset = 0;
        }
    }
}

int read_from_inode(int fs, int inode, int offset, char* read_buffer, int read_len) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }

    int end_offset = offset + read_len;

    int block_num = offset / superblock->BLOCK_SIZE;
    int block_offset = offset - (block_num * superblock->BLOCK_SIZE);
    int read_offset = 0;

    if (end_offset > get_inode_file_size(fs, inode)) {
        end_offset = get_inode_file_size(fs, inode);
    }

    while (read_len - read_offset > 0) {
        int block_id = get_inode_block_id(fs, inode, block_num);
        int block_pos = get_block_offset(fs, block_id);
        char read_char = fs_read(fs, block_pos + block_offset, 1)[0];
        read_buffer[read_offset] = read_char;
        block_offset += 1;
        read_offset += 1;
        int remaining_space = superblock->BLOCK_SIZE - block_offset;
        if (remaining_space <= 0) {
            block_num += 1;
            block_offset = 0;
        }
    }
}
int get_inode_block_id(int fs, int id, int pos) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }
    int addresses_per_block = superblock->BLOCK_SIZE / 8;
    free(superblock);

    int single_capacity = addresses_per_block;
    int double_capacity = single_capacity * addresses_per_block;
    int triple_capacity = double_capacity * addresses_per_block;


    if (pos < 12) { //DIRECT
        return get_block_id_at_pos(fs, pos, 0, get_inode_array(fs, id, pos));
    }

    pos -= 12;

    if (pos < single_capacity) {
        return get_block_id_at_pos(fs, pos, 1, get_inode_array(fs, id, 12));
    }

    pos -= single_capacity;

    if (pos < double_capacity) {
        return get_block_id_at_pos(fs, pos, 2, get_inode_array(fs, id, 13));
    }

    pos -= double_capacity;

    if (pos < triple_capacity) {
        return get_block_id_at_pos(fs, pos, 3, get_inode_array(fs, id, 14));
    }
    printf("Block ID out of max blocks for indoe\n");
    return -1;
}

int get_block_id_at_pos(int fs, int pos, int indirection, int block_id) {
    if (block_id == INODE_NULL_VALUE) { return -1; }

    if (indirection == 0) {
        return block_id;
    }

    Config* superblock = read_super_block(fs);
    if (superblock == NULL) { printf("Failed to open superblock"); return -1; }
    int pointers_per_block = superblock->BLOCK_SIZE / 8;
    free(superblock);

    int capacity = 1;
    for (int i = 1; i < indirection; i++) {
        capacity *= pointers_per_block;
    }
    int pointer_index = pos / capacity;
    int next_pos = pos % capacity;

    int block_offset = get_block_offset(fs, block_id);
    if (block_offset == -1) { printf("Failed to get block offset\n"); return -1; }

    int indirect_block_id = fs_read_num(fs, block_offset + pointer_index*8, 8);
    if (indirect_block_id == -1) { printf("Failed to read block offset\n"); return -1;}

    int nested_block = get_block_id_at_pos(fs, next_pos, indirection - 1, indirect_block_id);
    if (nested_block == -1) { printf("Recursion Failed\n"); return -1; }
    return nested_block;
}

int inode_add_block(int fs, int id) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }

    int allocated_blocks = get_inode_allocated_blocks(fs, id);
    int pos = allocated_blocks;
    int addresses_per_block = superblock->BLOCK_SIZE / 8;
    free(superblock);

    int single_capacity = addresses_per_block;
    int double_capacity = single_capacity * addresses_per_block;
    int triple_capacity = double_capacity * addresses_per_block;


    if (pos < 12) { //DIRECT
        return allocate_at_pos(fs, pos, 0, id ,pos);
    }

    pos -= 12;

    if (pos < single_capacity) {
        return allocate_at_pos(fs, pos, 1, id, 12);
    }

    pos -= single_capacity;

    if (pos < double_capacity) {
        return allocate_at_pos(fs, pos, 2, id,13);
    }

    pos -= double_capacity;

    if (pos < triple_capacity) {
        return allocate_at_pos(fs, pos, 3, id, 14);
    }
    printf("Reached max file block allocation\n");
    return -1;
}

int allocate_at_pos(int fs, int pos, int indirection, int id, int array_index) {
    int block_id = get_inode_array(fs, id, array_index);
    int status = allocate_block_in_array(fs, pos, indirection, block_id, id);
    if (status == -1) {
        return -1;
    }
    set_inode_array(fs, id, array_index, status);
    return 0;
}

int allocate_block_in_array(int fs, int pos, int indirection, int block_id, int id) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }
    int pointers_per_block = superblock->BLOCK_SIZE / 8;
    free(superblock);

    if (indirection == 0) {         //reference is direct allocate block
        if (block_id != INODE_NULL_VALUE) { printf("Can't allocate over direct block\n"); printf("direct block id is %d\n", block_id); return -1;}
        int allocated_blocks = get_inode_allocated_blocks(fs, id);
        set_inode_allocated_blocks(fs, id, allocated_blocks + 1);
        return allocate_block(fs);
    }

    if (block_id == INODE_NULL_VALUE) { //IF reference is null allocate block
        block_id = allocate_block(fs);
        if (block_id == -1) { return -1; }

        int status = format_block_as_indirect(fs, block_id);
        if (status == -1) { printf("Failed to format block\n"); return -1; }
    }

    int capacity = 1;
    for (int i = 1; i < indirection; i++) {
        capacity *= pointers_per_block;
    }
    int pointer_index = pos / capacity;
    int next_pos = pos % capacity;

    int block_offset = get_block_offset(fs, block_id);
    if (block_offset == -1) { printf("Failed to get block offset\n"); return -1; }

    int indirect_block_id = fs_read_num(fs, block_offset + pointer_index*8, 8);
    if (indirect_block_id == -1) { printf("Failed to read block offset\n"); return -1;}

    int allocated_block = allocate_block_in_array(fs, next_pos, indirection - 1, indirect_block_id, id);
    if (allocated_block == -1) { printf("Recursion Failed\n"); return -1; }

    fs_write_num(fs, block_offset + pointer_index*8, 8, allocated_block);
    return block_id;
}


int format_block_as_indirect(int fs, int block_id) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }
    int offset = get_block_offset(fs, block_id);

    for (int i =0; i < superblock->BLOCK_SIZE /8; i++) {
        fs_write_num(fs, offset + i*8, 8, INODE_NULL_VALUE);
    }
    free(superblock);
    return 0;
}

int allocate_block(int fs) {
    int block_id = get_available_block_id(fs);
    if (block_id == -1) {
        return -1;
    }
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }

    int block_offset = get_block_offset(fs, block_id);
    char block[superblock->BLOCK_SIZE];
    memset(block, 'A', superblock->BLOCK_SIZE);
    fs_write(fs, block_offset, superblock->BLOCK_SIZE, block);
    free(superblock);
    return block_id;
}

void create_dir(int fs, char* path, char* dname) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return;
    }

    int inode = create_inode(fs);
    if (inode == -1) {
        printf("Failed to create inode");
        free(superblock);
        return;
    }

    if (path[0] == '\0' ) {
        set_inode_file_type(fs, inode, 'd');
        set_inode_permissions(fs, inode, "rwx------");
        set_inode_owner_uid(fs, inode, 0);
        set_inode_group_uid(fs, inode, 0);

    } else {
        printf("Not Home DIR\n");
    }

    free(superblock);
}

int create_inode(int fs) {
    int node_id = get_available_inode_id(fs);
    if (node_id == -1) {
        return -1;
    }

    set_inode_bitmap(fs, node_id, 1);
    for (int i = 0; i < INODE_ARRAY_SIZE; i++) {
        set_inode_array(fs, node_id, i, INODE_NULL_VALUE);
    }
    set_inode_file_size(fs, node_id, 0);
    set_inode_inode_id(fs, node_id, node_id);
    set_inode_hard_links(fs, node_id, 0);
    set_inode_allocated_blocks(fs, node_id, 0);
    return node_id;
}

void set_inode_file_type(int fs, int id, char value) {
    int offset = get_inode_offset(fs, id);
    fs_write(fs, offset, 1, &value);
}
void set_inode_permissions(int fs, int id, char* value) {
    int offset = get_inode_offset(fs, id) + 1;
    fs_write(fs, offset, 9, value);;
}
void set_inode_owner_uid(int fs, int id, int value) {
    int offset = get_inode_offset(fs, id) + 10;
    fs_write_num(fs, offset, 8, value);
}
void set_inode_group_uid(int fs, int id, int value) {
    int offset = get_inode_offset(fs, id) + 18;
    fs_write_num(fs, offset, 8, value);
}
void set_inode_file_size(int fs, int id, int value) {
    int offset = get_inode_offset(fs, id) + 26;
    fs_write_num(fs, offset, 8, value);
}

int get_inode_file_size(int fs, int id) {
    int offset = get_inode_offset(fs, id) + 26;
    return fs_read_num(fs, offset, 8);
}

void set_inode_hard_links(int fs, int id, int value) {
    int offset = get_inode_offset(fs, id) + 34;
    fs_write_num(fs, offset, 8, value);
}
void set_inode_inode_id(int fs, int id, int value) {
    int offset = get_inode_offset(fs, id) + 42;
    fs_write_num(fs, offset, 8, value);
}
void set_inode_allocated_blocks(int fs, int id, int value) {
    int offset = get_inode_offset(fs, id) + 50;
    fs_write_num(fs, offset, 8, value);
}

int get_inode_allocated_blocks(int fs, int id) {
    int offset = get_inode_offset(fs, id) + 50;
    return fs_read_num(fs, offset, 8);
}
void set_inode_array(int fs, int id, int pos, int value) {
    int offset = get_inode_offset(fs, id) + 58 + 8*pos;
    fs_write_num(fs, offset, 8, value);
}

int get_inode_array(int fs, int id, int pos) {
    int offset = get_inode_offset(fs, id) + 58 + 8*pos;
    return fs_read_num(fs, offset, 8);
}

int get_available_inode_id(int fs) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }

    int inode_id = 0;

    for (int i = 0; i < superblock-> GROUPS; i++) {
        int offset = SUPER_BLOCK_SIZE + i* group_size(superblock);
        for (int j = 0; j < superblock-> INODE_NUM; j++) {
            int bit = fs_read_num(fs, offset, 1);
            if (bit == 0) {
                fs_write_num(fs, offset, 1, 1);
                free(superblock);
                return inode_id;
            }
            offset += 1;
            inode_id++;
        }
    }
    printf("No available inodes");
    free(superblock);
    return -1;
}

void set_inode_bitmap(int fs, int id, int value) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return;
    }

    int group = id / superblock-> INODE_NUM;
    int node_in_group = id % superblock-> INODE_NUM;
    int node_offset =  SUPER_BLOCK_SIZE + group * group_size(superblock) + node_in_group;
    fs_write_num(fs, node_offset, 1, value);
    free(superblock);
}

int get_inode_offset(int fs, int id) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }
    if (id < 0 || id >= superblock->INODE_NUM * superblock-> GROUPS) {
        free(superblock);
        printf("Cannot access inode out of range");
        return -1;
    }

    int group = id / superblock-> INODE_NUM;
    int node_in_group = id % superblock-> INODE_NUM;
    int node_offset =  SUPER_BLOCK_SIZE + group * group_size(superblock) + superblock->INODE_NUM + superblock->BLOCK_NUM + INODE_SIZE * node_in_group;
    free(superblock);
    return node_offset;
}

int get_block_offset(int fs, int id) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }
    if (id < 0 || id >= superblock->BLOCK_NUM * superblock->GROUPS) {
        free(superblock);
        printf("Cannot access data block out of range");
        return -1;
    }

    int group = id / superblock-> BLOCK_NUM;
    int block_in_group = id % superblock-> BLOCK_NUM;
    int block_offset =  SUPER_BLOCK_SIZE + group * group_size(superblock) + superblock->INODE_NUM + superblock->BLOCK_NUM + INODE_SIZE * superblock->INODE_NUM + superblock->BLOCK_SIZE * block_in_group;
    free(superblock);
    return block_offset;
}

int get_available_block_id(int fs) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock");
        return -1;
    }

    int block_id = 0;

    for (int i = 0; i < superblock-> GROUPS; i++) {
        int offset = SUPER_BLOCK_SIZE + superblock->INODE_NUM + i* group_size(superblock);
        for (int j = 0; j < superblock-> BLOCK_NUM; j++) {
            int bit = fs_read_num(fs, offset, 1);
            if (bit == 0) {
                fs_write_num(fs, offset, 1, 1);
                free(superblock);
                return block_id;
            }
            offset += 1;
            block_id++;
        }
    }
    printf("No available data blocks\n");
    free(superblock);
    return -1;
}

int set_block_bitmap(int fs, int id, int value) {
    Config* superblock = read_super_block(fs);
    if (superblock == NULL) {
        printf("Failed to open superblock\n");
        return -1;
    }

    int group = id / superblock-> BLOCK_NUM;
    int node_in_group = id % superblock-> BLOCK_NUM;
    int node_offset =  SUPER_BLOCK_SIZE + group * group_size(superblock) + superblock->INODE_NUM + node_in_group;
    fs_write_num(fs, node_offset, 1, value);
    free(superblock);
    return 0;
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

int check_config(const Config* config) {

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

    int needed_space = SUPER_BLOCK_SIZE + group_size(config) * config->GROUPS;

    if (needed_space > config->DISK_SIZE || needed_space < 0) {
        printf("Config does not fit in disk size. DISK_SIZE: %d CONFIG SIZE: %d\n", config->DISK_SIZE, needed_space);
        return -1;
    }
    printf("Valid Config. DISK_SIZE: %d CONFIG SIZE: %d\n", config->DISK_SIZE, needed_space);
    return 0;
}

int group_size(const Config* config) {
    int space_per_group = 0;
    space_per_group += config->INODE_NUM; //Space for inode bitmap
    space_per_group += config->BLOCK_NUM; //space for datablock bitmap
    space_per_group += config->INODE_NUM * INODE_SIZE; //space for inodes
    space_per_group += config->BLOCK_NUM * config->BLOCK_SIZE; //space for blocks
    return space_per_group;
}