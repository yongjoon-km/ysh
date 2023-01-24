#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ysh_loop();

int main(int argc, char** argv) {

    ysh_loop();
    return 0;
}

void ysh_loop() {

    char *line = NULL;
    size_t len = 0;
    ssize_t size;
    while ((size = getline(&line, &len, stdin)) != -1) {
        if (strcmp(line, "exit\n") == 0) {
            break;
        }
        printf("%s\n", line);
    }
    return;
}

