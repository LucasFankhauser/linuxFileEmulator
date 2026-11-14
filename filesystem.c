//
// Created by lucas on 11/8/2026.
//

#include "filesystem.h"

#include "fs_utility.h"

const int SUPER_BLOCK_SIZE = 40;
const int INODE_SIZE = 178;
const int INODE_ARRAY_SIZE = 15;
const int INODE_NULL_VALUE = 99999990;
const int MAX_FILE_NAME = 24;


int get_max_file_name() {
    return MAX_FILE_NAME;
}
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
        return open_fs();
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
    create_root_directory(fs);
    create_dir(fs, "/", "~");
    close(fs);
    return 0;
}

int write_word_to_inode(int fs, int inode, int offset, char* write_buffer, int write_len) {
    char str_buffer[write_len];
    int hit_end = 0;
    for (int i = 0; i < write_len; i++) {
        if (write_buffer[i] == '\0') {
            hit_end = 1;
        }
        if (hit_end == 1) {
            str_buffer[i] = '?';
        } else {
            str_buffer[i] = write_buffer[i];
        }
    }
    return write_to_inode(fs, inode, offset, str_buffer, write_len);
}

int read_word_from_inode(int fs, int inode, int offset, char* read_buffer, int read_len) {
    if (read_from_inode(fs, inode, offset, read_buffer, read_len) == -1) {
        return -1;
    }

    for (int i = 0; i < read_len; i++) {
        if (read_buffer[i] == '?') {
            read_buffer[i] = '\0';
            break;
        }
    }
    return 0;
}

int write_num_to_inode(int fs, int inode, int offset, int value, int space) {
    char int_as_str[space + 1];
    snprintf(int_as_str, sizeof(int_as_str), "%d", value);
    for (int i = 0; i < space; i++) {
        if (!(int_as_str[i] >= '0' && int_as_str[i] <= '9' )) {
            int_as_str[i] = '?';
        }
    }
    return write_to_inode(fs, inode, offset, int_as_str, space);
}

int read_num_from_inode(int fs, int inode, int offset, int read_len) {
    char str_buffer[read_len];
    if (read_from_inode(fs, inode, offset, str_buffer, read_len) == -1) {
        return -1;
    }
    char int_as_str[read_len + 1];
    int_as_str[read_len] = '\0';
    for (int i = 0; i < read_len; i++) {
        if (!(int_as_str[i] >= '0' && int_as_str[i] <= '9' )) {
            int_as_str[i] = '\0';
            break;
        }
    }
    return atoi(str_buffer);
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
    if (offset == -1) {
        offset = get_inode_file_size(fs, inode);
    }
    int end_offset = offset + write_len;
    int allocated_space = get_inode_allocated_blocks(fs, inode) * superblock->BLOCK_SIZE;
    //create space for write
    while (end_offset > allocated_space) {
        if (inode_add_block(fs, inode) == -1) {
            free(superblock);
            return -1;
        }
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
        int remaining_space = superblock->BLOCK_SIZE - block_offset;
        int remaining_data = write_len - write_offset;

        int chunk_size = remaining_space < remaining_data ? remaining_space : remaining_data;

        fs_write(fs, block_pos + block_offset, chunk_size, write_buffer + write_offset);
        write_offset += chunk_size;

        block_num++;
        block_offset = 0;
    }
    free(superblock);
    return 0;
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
        read_len  = get_inode_file_size(fs, inode) - offset;
    }

    while (read_offset < read_len) {
        int block_id = get_inode_block_id(fs, inode, block_num);
        int block_pos = get_block_offset(fs, block_id);

        int remaining_space = superblock->BLOCK_SIZE - block_offset;
        int remaining_data = read_len - read_offset;

        int chunk_size = remaining_space < remaining_data ? remaining_space : remaining_data;

        char* data = fs_read(fs, block_pos + block_offset, chunk_size);

        if (data == NULL) {
            free(superblock);
            return -1;
        }

        for (int i = 0; i < chunk_size; i++) {
            read_buffer[read_offset + i] = data[i];
        }

        free(data);

        read_offset += chunk_size;
        block_num++;
        block_offset = 0;
    }
    if (read_offset < end_offset) {
        read_buffer[read_offset] = '\0';
    }
    free(superblock);
    return 0;

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

void create_root_directory(int fs) {
    int inode = create_inode(fs);
    if (inode == -1) {
        printf("Failed to create inode\n");
        return;
    }
    set_inode_file_type(fs, inode, 'd');
    set_inode_permissions(fs, inode, "rwx------");
    set_inode_owner_uid(fs, inode, 0);
    set_inode_group_uid(fs, inode, 0);
    write_word_to_inode(fs, inode, -1, ".",MAX_FILE_NAME);
    write_num_to_inode(fs, inode, -1, inode,8);
    write_word_to_inode(fs, inode, -1, "..",MAX_FILE_NAME);
    write_num_to_inode(fs, inode, -1, inode,8);
}

void create_dir(int fs, const char* path, char* dname) {
    int parent_node_id = follow_path(fs, path);
    if (parent_node_id == -1) {
        printf("Failed to create dir");
        return;
    }

    if (get_file(fs, parent_node_id, dname) != -1) {
        printf("Directory already exists\n");
    }

    int inode = create_inode(fs);
    if (inode == -1) {
        printf("Failed to create dir\n");
        return;
    }

    set_inode_file_type(fs, inode, 'd');
    set_inode_permissions(fs, inode, "rwxrwxrwx");
    set_inode_owner_uid(fs, inode, 0);
    set_inode_group_uid(fs, inode, 0);
    write_word_to_inode(fs, inode, -1, ".",MAX_FILE_NAME);
    write_num_to_inode(fs, inode, -1, inode,8);
    write_word_to_inode(fs, inode, -1, "..",MAX_FILE_NAME);
    write_num_to_inode(fs, inode, -1, parent_node_id,8);

    write_word_to_inode(fs, parent_node_id, -1, dname,MAX_FILE_NAME);
    write_num_to_inode(fs, parent_node_id, -1, inode,8);

}

int get_file(int fs, int dir_node, char* fname) {
    int paths_in_dir = get_inode_file_size(fs, dir_node) / (MAX_FILE_NAME + 8);

    for (int i = 2; i < paths_in_dir; i++) {
        char filename[MAX_FILE_NAME + 1];
        filename[0] = '\0';
        read_word_from_inode(fs, dir_node, (MAX_FILE_NAME + 8)*i, filename, MAX_FILE_NAME );
        if (str_match(filename, fname) == 0) {
            return read_num_from_inode(fs, dir_node, (MAX_FILE_NAME + 8)*i + MAX_FILE_NAME, MAX_FILE_NAME);
        }
    }
    return -1;
}

void create_file(int fs, const char* path, char* fname) {
    int parent_node_id = follow_path(fs, path);
    printf("parent_node_id is %d\n", parent_node_id);
    if (parent_node_id == -1) {
        printf("Failed to create dir");
        return;
    }

    if (get_file(fs, parent_node_id, fname) != -1) {
        printf("File already exists\n");
    }

    int inode = create_inode(fs);
    if (inode == -1) {
        printf("Failed to create dir");
        return;
    }

    set_inode_file_type(fs, inode, 'f');
    set_inode_permissions(fs, inode, "rwxrwxrwx");
    set_inode_owner_uid(fs, inode, 0);
    set_inode_group_uid(fs, inode, 0);

    write_word_to_inode(fs, parent_node_id, -1, fname,MAX_FILE_NAME);
    write_num_to_inode(fs, parent_node_id, -1, inode,8);
}

int follow_path(int fs, const char* path) {

    //printf("Following path: %s\n", path);

    char current_dir[MAX_FILE_NAME + 1];
    current_dir[MAX_FILE_NAME] = '\0';
    get_path_name(0, path, current_dir);

    char last_dir[MAX_FILE_NAME + 1];
    last_dir[MAX_FILE_NAME] = '\0';

    get_path_name(0, path, last_dir);

    if (!(path[0] == '\0' || path[0] == '/' || path[0] == '\\')) {
        printf("Path does not start at root directory\n");
        return -1;
    }

    if (get_path_name(1, path, current_dir) == 0) { //hit end
        return 0; //root
    }



    int current_inode = 0;
    int next_inode = 0;

    int path_index = 0;


    while (1) { //sets current dir to the next path index stops when hit end
        get_path_name(path_index, path, last_dir);
        int path_res = get_path_name(path_index + 1, path, current_dir);

        int found_inode = 0;

        if (!(get_inode_file_type(fs, next_inode)[0] == 'd') ) { //check if entering dir
            printf("Cannot enter file\n");
            return -1;
        }


        char name_in_dir[MAX_FILE_NAME + 1];
       // printf("Inodes in %s\n", last_dir);

        int paths_in_dir = get_inode_file_size(fs, current_inode) / (MAX_FILE_NAME + 8);

        for (int i = 0; i < paths_in_dir; i++) { //checks each file in current directory looking for

            read_word_from_inode(fs, current_inode, (MAX_FILE_NAME + 8) * i, name_in_dir, MAX_FILE_NAME);
            name_in_dir[MAX_FILE_NAME] = '\0';

            //printf("Inode %d: %s\n", i, name_in_dir);

            if (str_match(name_in_dir, current_dir) == 0) {
                next_inode = read_num_from_inode(fs, current_inode, (MAX_FILE_NAME + 8) * i + MAX_FILE_NAME, 8);

                //printf("Found file %s in %s\n", name_in_dir, last_dir);
                current_inode = next_inode;
                path_index++;
                found_inode = 1;
                break;
            }
        }

        if (path_res == 0) {
            //printf("%d\n", current_inode);
            return current_inode;
        }

        if (!found_inode) {
            printf("Could not find %s in directory %s\n", current_dir, last_dir);
            return -1;
        }
    }

}


int get_dir_path(int fs, int inode, char** path) {
    if (get_inode_file_type(fs, inode)[0] != 'd') {
        return -1;
    }

    char filename[MAX_FILE_NAME + 1];  //do it once to get path size
    int path_size = 0;
    int current_inode = inode;
    while (current_inode != 0) {
        int parent_inode = read_num_from_inode(fs, current_inode, MAX_FILE_NAME + 8 + MAX_FILE_NAME, 8);
        filename[MAX_FILE_NAME] = '\0';
        int paths_in_dir = get_inode_file_size(fs, parent_inode) / (MAX_FILE_NAME + 8);

        for (int i = 0; i < paths_in_dir; i++) {
            if (read_num_from_inode(fs, parent_inode, i*(MAX_FILE_NAME + 8) + MAX_FILE_NAME, 8) == current_inode) {
                read_word_from_inode(fs, parent_inode, i*(MAX_FILE_NAME + 8), filename, MAX_FILE_NAME);
                break;
            }
        }
        path_size += get_len(filename, -1) + 1;

        current_inode = parent_inode;
    }

    path_size += get_len(filename, -1) -1;

    *path = malloc(sizeof(char) * (path_size + 1));
    (*path)[path_size] = '\0';
    current_inode = inode;
    int index = 1;
    while (current_inode != 0) {
        int parent_inode = read_num_from_inode(fs, current_inode, MAX_FILE_NAME + 8 + MAX_FILE_NAME, 8);
        filename[MAX_FILE_NAME] = '\0';
        int paths_in_dir = get_inode_file_size(fs, parent_inode) / (MAX_FILE_NAME + 8);

        for (int i = 0; i < paths_in_dir; i++) {
            if (read_num_from_inode(fs, parent_inode, i*(MAX_FILE_NAME + 8) + MAX_FILE_NAME, 8) == current_inode) {
                read_word_from_inode(fs, parent_inode, i*(MAX_FILE_NAME + 8), filename, MAX_FILE_NAME);
                break;
            }
        }
        int file_name_len = get_len(filename, -1);
        for (int i = 0; i < file_name_len; i++) {
            (*path)[path_size - index] = filename[file_name_len - i - 1];
            index++;
        }
        (*path)[path_size - index] = '/';
        index++;

        current_inode = parent_inode;
    }
}

int get_path_name(int index, const char* path, char* file_name) { //gets next dir in path
    int paths = 0;
    int i = 0;
    int name_index = 0;
    while (path[i] != '\0') {
        if (path[i] == '\'' || path[i] == '/') {
            paths++;
            i++;
            continue;
        }
        if (paths == index ) {
            file_name[name_index] = path[i];
            name_index++;
        }
        i++;
    }
    file_name[name_index] = '\0';
    if (name_index < index) {
        return 0; //0 means hit end
    }
    return 1;
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

char* get_inode_file_type(int fs, int id) {
    int offset = get_inode_offset(fs, id);
    return fs_read(fs, offset, 1);
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
        int offset = SUPER_BLOCK_SIZE + i * group_size(superblock);
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
    printf("No available inodes\n");
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
        printf("Cannot access data block out of range: %d\n", id);
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