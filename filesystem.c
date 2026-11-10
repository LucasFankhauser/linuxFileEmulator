//
// Created by lucas on 11/8/2026.
//

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int parse(int file, char* buffer, char* attr);
int str_match(char* str1, char* str2);

int init() {
    printf("Initiating Filesystem\n");

    int file = open( "superblock", O_RDWR | O_CREAT | O_APPEND, 0700 );
    if (file == -1) {
        printf("Error opening superblock\n");
        return -1;
    }

    char output[1024];
    parse(file, output, "DISK_SIZE");

    return 0;
}

int parse(int file, char* buffer, char* attr) {
    lseek(file, 0, SEEK_SET);
    char temp_buffer[1024];
    int attr_size = snprintf( temp_buffer, 1023, "%s", attr );

    while (read(file, temp_buffer, attr_size) == attr_size) {
        temp_buffer[attr_size] = '\0';
        if (str_match(temp_buffer, attr) == 0) {
            char char_buffer;
            int i = 0;
            while (char_buffer != '\n') {
                if ( char_buffer >= '0' && char_buffer <= '9') {
                    buffer[i] = char_buffer;
                    i++;
                }
                read(file, &char_buffer, 1);
            }
            buffer[i] = '\0';
            printf("%s\n", buffer);
        }

        lseek(file, -attr_size + 1, SEEK_CUR);
    }

    return -1;
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