#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void ysh_loop();

int main(int argc, char** argv) {

    ysh_loop();
    return 0;
}

void execute_command(char *command, char **argv) {

    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {

        printf("ERROR: forking child process failed\n");
        exit(1);
    }
    else if (pid == 0) {
        if (execvp(command, argv) < 0) {
            printf("ERROR: exec child process failed\n");
            exit(1);
        }
    }
    else {
        while (wait(&status) != pid);
    }
}

char **tokenize(char *line) {
    char *token = NULL;
    size_t bufsize = 1;
    char **tokens = (char **)malloc(sizeof(char *) * bufsize);
    int i = 0;
    token = strtok(line, " ");
    while (token != NULL) {

        if (i + 1 >= bufsize) {
            bufsize += 1;
            tokens = (char **)realloc(tokens, sizeof(char *) * bufsize);
            tokens[i] = (char *)malloc(sizeof(char) * (strlen(token)+1));
        }
        strcpy(tokens[i], token);
        i++;
        token = strtok(NULL, " ");
    }
    return tokens;
}

void ysh_loop() {

    char *line = NULL;
    char **tokens = NULL;
    size_t len = 0;
    ssize_t size;
    printf("> ");
    while ((size = getline(&line, &len, stdin)) != -1) {
        if (strcmp(line, "exit\n") == 0) {
            break;
        }
        line[strlen(line)-1] = NULL;
        tokens = tokenize(line);
        execute_command(*tokens, tokens);
        printf("> ");
    }
    return;
}

