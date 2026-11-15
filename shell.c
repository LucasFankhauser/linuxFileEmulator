//
// Created by Lucas on 11/11/2026.
//


#include "shell.h"

Command commands[32];
int total_commands = 0;
int last_status = 1;

const int MAX_INPUT_LENGTH = 4096;
int fs = -1;
char* current_path;

void print_command_info(int cid) {
    Command command = commands[cid];
    printf("%s %s\n", command.name, command.format);
    if (command.description[0] != '\0') {
        printf("%s\n", command.description);
    }
    printf("\n");
}

void add_command(char* name, int arg_num, CommandFunc func, char* format, char* description) {
    commands[total_commands].name = name;
    commands[total_commands].func = func;
    commands[total_commands].arg_num = arg_num;
    commands[total_commands].format = format;
    commands[total_commands].description = description;
    total_commands++;
}

int cexit(int arg_num, char** args) {
    return -1;
}

int ccd(int arg_num, char** args) {
    char* new_path = strdup(args[1]);
    if (new_path == NULL) return 0;

    if (format_path(&new_path) == -1) {
        free(new_path);
        return 0;
    }

    free(current_path);
    current_path = new_path;
    return 1;
}

int cls(int arg_num, char** args) {
    int node = follow_path(fs, current_path);
    if (node == -1) {
        return 0;
    }

    const int MAX_FILE_NAME = get_max_file_name();
    int paths_in_dir = get_inode_file_size(fs, node) / (get_max_file_name() + 8);
    for (int i = 2; i < paths_in_dir; i++) {
        char filename[MAX_FILE_NAME + 1];
        filename[MAX_FILE_NAME] = '\0';
        read_word_from_inode(fs, node, (MAX_FILE_NAME + 8)*i, filename, MAX_FILE_NAME );
        int read_node = read_num_from_inode(fs, node, (MAX_FILE_NAME + 8)*i + MAX_FILE_NAME, 8);
        if (read_node != get_inode_null_value()) {
            printf("%s\n", filename);
        }
    }
    return 1;
}

int cmkdir(int arg_num, char** args) {
    create_dir(fs, current_path, args[1]);
    return 1;
}

int ctouch(int arg_num, char** args) {
    create_file(fs, current_path, args[1]);
    return 1;
}

int ccat(int arg_num, char** args) {
    int node = follow_path(fs, current_path);
    if (node == -1) {
        return 0;
    }
    int file = get_file(fs, node, args[1]);
    if (file == -1) {
        printf("Could not find file in directory\n");
        return 0;
    }
    char filetype = get_inode_file_type(fs, file)[0];
    if (filetype != 'f') {
        printf("can only read from files\n");
        return 0;
    }
    int file_size = get_inode_file_size(fs, file);
    char buffer[file_size+1];
    buffer[0] = '\0';
    buffer[file_size] = '\0';
    read_from_inode(fs, file, 0, buffer, file_size);
    printf("%s\n", buffer);
    return 1;
}

int cecho(int arg_num, char **args) {
    int node = follow_path(fs, current_path);
    if (node == -1) {
        return 0;
    }
    int file = get_file(fs, node, args[1]);
    if (file == -1) {
        printf("Could not find file in directory\n");
        return 0;
    }

    char filetype = get_inode_file_type(fs, file)[0];
    if (filetype != 'f') {
        printf("can only write to files\n");
        return 0;
    }

    write_to_inode(fs, file, -1, args[2], get_len(args[2], -1));
    return 1;
}

int chelp(int arg_num, char** args) {
    for (int i = 0; i < total_commands; i++) {
        print_command_info(i);
    }
    return 1;
}

int cstat(int arg_num, char** args) {
    int node = follow_path(fs, current_path);
    if (node == -1) {
        return 0;
    }
    int file = get_file(fs, node, args[1]);
    if (file == -1) {
        printf("Could not find file in directory\n");
        return 0;
    }

    print_inode_stat(fs, file);
    return 1;
}

int cman(int arg_num, char** args) {
    char* command_input = args[1];
    int found_a_command = 0;
    for (int i = 0; i < total_commands; i++) {
        Command command = commands[i];
        if (str_match(command.name, command_input) == 0) {
            print_command_info(i);
            found_a_command = 1;
        }
    }
    if (!found_a_command) {
        printf("%s is not a known command\n", command_input);
        return 0;
    }
    return 1;
}

int crm(int arg_num, char** args) {
    if (delete_file(fs, current_path, args[1]) == -1) {
        return 0;
    }
    return 1;
}

int crrm(int arg_num, char** args) { //recursive remove
    int node = follow_path(fs, current_path);
    if (node == -1) {
        return 0;
    }
    int file = get_file(fs, node, args[2]);
    if (file == -1) {
        printf("Could not find file in directory\n");
        return 0;
    }

    char filetype = get_inode_file_type(fs, file)[0];
    if (filetype == 'd') {
        if (delete_directory(fs, current_path, args[2], 0) == -1) {
            return 0;
        }
    } else {
        if (delete_file(fs, current_path, args[2]) == -1) {
            return 0;
        }
    }
    return 1;
}

int crmdir(int arg_num, char** args) {
    int parent_dir_id = follow_path(fs, current_path);
    if (parent_dir_id == -1) {
        return 0;
    }

    int sub_dir_id = get_file(fs, parent_dir_id, args[1]);
    if (sub_dir_id == -1) {
        printf("Cannot find subdirectory: %s\n", args[1]);
        return 0;
    }
    if (delete_directory(fs, current_path,  args[1], 1) == -1) {
        return 0;
    }
    return 1;
}

int cpwd(int arg_num, char** args) {
    printf("Cannot find subdirectory: %s\n", current_path);
    return 1;
}

int cstatus(int arg_num, char** args) {
    printf("%d\n", !last_status);
    return 1;
}



void init_commands() {
    add_command("help", 0, chelp, "","Displays all commands");
    add_command("exit", 0, cexit, "", "Closes filesystem and exits shell");
    add_command("cd", 1, ccd, "[path]","Changes current path use .. to reach parent or / to set absolute path");
    add_command("ls", 0, cls, "","Prints contents of current directory");
    add_command("mkdir", 1, cmkdir, "[dname]","Creates a directory in the current directory");
    add_command("touch", 1, ctouch, "[fname]","Creates a file in the current directory");
    add_command("cat", 1, ccat, "[fname]","Prints the content of a file");
    add_command("echo", 2, cecho, "[fname] [data]","Appends data to a file");
    add_command("stat", 1, cstat, "[name]","Prints inode metadata for file or directory");
    add_command("man", 1, cman, "[command]","Prints command description for given command");
    add_command("rm", 1, crm, "[fname]","Deletes a file");
    add_command("rm", 2, crrm, "[-r] [dname]","Deletes file or subdirectory recursively");
    add_command("rmdir", 1, crmdir, "[dname]","Deletes an empty subdirectory");
    add_command("pwd", 0, cpwd, "","Prints the current directory");
    add_command("$?", 0, cstatus, "","Prints exit status of last command");

}


void open_shell(int new_fs) {
    printf("Shell initialized\n");
    printf("-----------------------------------------------------\n");
    printf("\n");
    init_commands();
    fs = new_fs;
    int running = 1;

    char* starting_path = "/~";

    current_path = malloc(get_len(starting_path, -1)*sizeof(char) + 1);
    for (int i = 0; i < get_len(starting_path, -1); i++) {
        current_path[i] = starting_path[i];
    }
    current_path[get_len(starting_path, -1)] = '\0';


    while (running == 1) {
        printf("filesystem:%s ", current_path);
        char input_buffer[MAX_INPUT_LENGTH];
        if (fgets(input_buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
            continue;
        }
        int status = handle_input(input_buffer);
        last_status = status;
        if (status == -1) {
            running = 0;
        }

    }

    printf("Shell closed\n");
}


int handle_input(char *input) {
    int words = words_in_input(input);
    if (words == 0) {
        printf("Empty input\n");
        return 0;
    }
    char** args = parse_args(input, words);
    int status = run_command(args, words);
    free_args(args, words);
    return status;
}


int run_command(char **args, int arg_num) {
    char* command_input = args[0];
    int found_a_command = 0;

    if (command_input == NULL) {
        printf("Empty command\n");
        return 0;
    }
    for (int i = 0; i < total_commands; i++) {
        Command command = commands[i];
        if (str_match(command.name, command_input) == 0) {
            int required_args = command.arg_num;
            found_a_command = 1;
            if (required_args == arg_num -1) {
                return commands[i].func(arg_num, args);
            }
        }
    }

    if (!found_a_command) {
        printf("Unrecognized command: %s\n", command_input);
        return 0;
    }
    printf("Unrecognized number of arguments for: %s\n", command_input);
    return 0;
}


int format_path(char** path) {
    char* old_path = *path;
    if ((old_path)[0] == '/' || (old_path)[0] == '\\') { //path is absalute
        int node_at_path = follow_path(fs, old_path);
        if (node_at_path == -1) {
            return -1;
        }
        get_dir_path(fs, node_at_path, path);
        free(old_path);
        return 0;
    }

    int cd_len = get_len(current_path, -1);
    int path_len = get_len(*path, -1);
    int new_path_length = cd_len + 1 + path_len + 1;

    char* new_path = malloc(new_path_length * sizeof(char));
    if (new_path == NULL) {
        return -1;
    }

    for (int i = 0; i < cd_len; i++) {
        new_path[i] = current_path[i];
    }

    new_path[cd_len] = '/';

    for (int i = 0; i < path_len; i++) {
        new_path[cd_len + 1 + i] = (*path)[i];
    }

    new_path[new_path_length - 1] = '\0';

    int node_at_path = follow_path(fs, new_path);
    if (node_at_path == -1) {
        free(new_path);
        return -1;
    }

    get_dir_path(fs, node_at_path, path);
    free(old_path);
    return 1;
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

//to add quotes i also need to change word counter
char** parse_args(char *input, int word_count) {
    char** args = calloc(word_count + 1, sizeof(char*));
    if (args == NULL) {
        return NULL;
    }

    int word_index = 0;
    int word_start = -1;
    for (int i = 0; i <= MAX_INPUT_LENGTH && input[i] != '\0'; i++) {
        if (!isspace(input[i]) && word_start == -1) {
            word_start = i;
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