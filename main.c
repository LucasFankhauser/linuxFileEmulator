#include <stdio.h>
#include "filesystem.h"
#include "shell.h"

int main(void) {
    int fs = init();
    if (fs == -1) {
        printf("Error opening filesystem\n");
        return -1;
    }
    printf("File system initialized\n");
    open_shell(fs);

    return 0;
}