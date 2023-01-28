#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void ysh_loop();

int main(int argc, char** argv) {

    ysh_loop();
    return 0;
}

void execute_command(char *command1, char **argv1, char *command2, char **argv2) {

    pid_t pid;
    pid_t pid2;
    int status;
    int fd[2];
    if(pipe(fd) < 0) {
        printf("ERROR: creating pipe failed\n");
        exit(1);
    }

    if ((pid = fork()) < 0) {
        printf("ERROR: forking child process failed\n");
        exit(1);
    }
    else if (pid == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        if (execvp(command1, argv1) < 0) {
            printf("ERROR: exec child process failed\n");
            exit(1);
        }
    }
    else {
        if ((pid2 = fork()) < 0) {
            printf("ERROR: forking child process2 failed\n");
            exit(1);
        }
        else if (pid2 == 0) {
            if (command2 != NULL) {
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                if (execvp(command2, argv2) < 0) {
                    printf("ERROR: exec child process2 failed\n");
                    exit(1);
                }
            }
        } else {
            close(fd[0]);
            close(fd[1]);
            while (wait(&status) != pid);
        }
    }
}

int find_pipe(char **tokens) {

    int i = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "|") == 0) {
            return i;
        }
        i++;
    }

    return -1;
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

int sizeof_char_array(char **array) {

    int size = 0;
    for (size = 0; array[size] != NULL; size++);
    return size;
}

char ***split_commands(char **tokens) {

    size_t bufsize = 1;
    int token_len = sizeof_char_array(tokens);
    char ***commands = (char ***) malloc(sizeof(char **) * bufsize);

    int command_cnt = 0;
    int i = 0;
    int pipe_index = 0;
    while (1) {
        if (i + 1 > bufsize) {
            bufsize += 1;
            commands = (char ***) realloc(commands, sizeof(char **) * bufsize);
        }
        int pipe_index = find_pipe(tokens + i);
        if (pipe_index == -1) {
            char **command = (char **) malloc(sizeof(char *) * (token_len - i));
            command = tokens + i;
            commands[command_cnt] = command;
            break;
        }
        char **command = (char **) malloc(sizeof(char *) * (pipe_index-i));
        command = tokens + i;
        tokens[pipe_index] = NULL;

        commands[command_cnt] = command;
        i = pipe_index + 1;
        command_cnt++;
    }
        
    return commands;
}

void launch(char ***commands) {

    // TODO: Change to run arbitrary number of commands
    execute_command(*commands[0], commands[0], *commands[1], commands[1]);
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
        int pipe_index = find_pipe(tokens);
        char ***commands = split_commands(tokens);
        launch(commands);
        printf("> ");
    }
    return;
}

