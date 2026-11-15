#include <stdio.h>
#include "filesystem.h"
#include "shell.h"

int main(void) {
    printf("\n");
    printf("-----------------------------------------------------\n");
    int fs = init();
    if (fs == -1) {
        printf("Error opening filesystem\n");
        return -1;
    }
    printf("File system initialized\n");
    open_shell(fs);

    return 0;
}

//FEATURES THAT COULD BE ADDED
//using quotes in command args so you can write strings with spaces
//permissions ownership and gid is tracked but has no functionailty
//more commands
//find	Search for files/directories
//mv	Move or rename files
//cp	Copy files
//ln	Create hard link