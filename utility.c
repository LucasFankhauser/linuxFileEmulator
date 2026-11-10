//
// Created by Lucas on 11/9/2026.
//
#include "utility.h"

int parse_get(int file, char* buffer, char* attr) {

    long pos = get_pos(file, attr);
    if (pos == -1) {
        printf("Error getting parse: %s\n", attr);
        return -1;
    }

    lseek(file, pos, SEEK_SET);

    char char_buffer = '\0';
    int i = 0;
    while (read(file, &char_buffer, 1) == 1 && char_buffer != '\n') {
        if (char_buffer >= '0' && char_buffer <= '9') {
            buffer[i++] = char_buffer;
        }
    }
    buffer[i] = '\0';

    return 0;
}

long get_pos(int file, char* attr) {
    lseek(file, 0, SEEK_SET);
    char temp_buffer[1024];
    int attr_size = get_len( attr, 1024 );

    if (attr_size == -1) {
        printf("Error getting attribute length: %s\n", attr);
    }

    while (read(file, temp_buffer, attr_size) == attr_size) {
        temp_buffer[attr_size] = '\0';
        if (str_match(temp_buffer, attr) == 0) {
            return lseek(file, 0, SEEK_CUR);
        }
        lseek(file, -attr_size + 1, SEEK_CUR);
    }
    return -1;
}

int get_len(char* char_array, int max) {
    int len = 0;
    while (char_array[len] != '\0') {
        len++;
        if (len == max) {
            return -1;
        }
    }
    return len;
}

int parse_set(int file, char* value, char* attr) {

    int value_size = get_len( value, 1024);
    if (value_size == -1) {
        return -1;
    }

    long spos = get_pos(file, attr);
    if (spos == -1) {
        printf("0 Error setting parse: %s\n", attr);
        return -1;
    }


    char char_buffer = '\0';

    long epos = -1;
    lseek(file, spos, SEEK_SET);
    while (read(file, &char_buffer, 1) == 1) {
        if ( char_buffer == '\n') {
            epos = lseek(file, 0, SEEK_CUR);
            break;
        }
    }

    if (epos == -1) {
        epos = lseek(file, 0, SEEK_END);
    }

    long fsize = lseek(file, 0, SEEK_END);

    char* first = malloc((spos)*sizeof(char));
    char*last = malloc((fsize-epos)*sizeof(char));
    if (first == NULL || last == NULL) {
        free(first);
        free(last);
        printf("Failed malloc\n");
        return -1;
    }

    lseek(file, 0, SEEK_SET);
    read(file, first, spos);

    lseek(file, epos, SEEK_SET);
    read(file, last, fsize - epos);

    if (ftruncate(file, 0) == -1) {
        printf("Error truncating file");
        free(first);
        free(last);
        return -1;
    }

    lseek(file, 0, SEEK_SET);
    write(file, first, spos);
    write(file, " = ", 3);


    write(file, value, value_size);
    write(file, "\n", 1);
    write(file, last, fsize-epos);
    free(first);
    free(last);
    return 0;
}

int str_match(char* str1, char* str2) {
    int i = 0;
    while (str1[i] == str2[i]) {
        if (str1[i] == '\0') {
            return 0;
        }
        i++;
    }
    return -1;
}
