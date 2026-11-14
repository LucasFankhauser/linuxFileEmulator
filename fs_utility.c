//
// Created by Lucas on 11/9/2026.
//

#include "fs_utility.h"

#include <stdlib.h>



int fs_read_num(int fs, int offset, int space) {
    char* num_string = fs_read(fs, offset, space);
    if (num_string == NULL) {
        return -1;
    }
    int num = atoi(num_string);
    free(num_string);
    return num;
}

char* fs_read(int fs, int offset, int space) {
    lseek(fs, offset, SEEK_SET);
    char read_string[space];
    char* num_string = malloc(space*sizeof(char) + 1);

    if (num_string == NULL) {
        return NULL;
    }

    read(fs, read_string, space);
    int i = 0;
    while (i < space) {
        if ((read_string[i] == '?')) {
            break;
        }
        num_string[i] = read_string[i];
        i++;
    }

    num_string[i] = '\0';
    return num_string;
}



void fs_write_num(int fs, int offset, int space, int num ) {
    char num_string[space + 1];
    snprintf(num_string, sizeof(num_string), "%d", num);
    fs_write(fs, offset, space, num_string);
}

void fs_write(int fs, int offset, int space, char* line) {
    lseek(fs, offset, SEEK_SET);

    char write_string[space];

    int hit_end = 0;

    for (int i = 0; i < space; i++) {
        if (line[i] == '\0') {
            hit_end = 1;
        }

        if (!hit_end) {
            write_string[i] = line[i];
        } else {
            write_string[i] = '?';
        }
    }

    write(fs, write_string, space);
}
