#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

void ysh_loop();

int main(int argc, char** argv) {

    ysh_loop();
    return 0;
}

void close_all_pipes(int pipes[][2], int n) {
    int i = 0;
    for (i = 0; i < n; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

void execute_command_loop(char ***commands) {
    int command_index = 0;
    
    int command_count = 0;
    for (command_count = 0; commands[command_count] != NULL; command_count++);

    int pipes[command_count][2];

    int pipe_index = 0;
    for (pipe_index = 0; pipe_index < command_count; pipe_index++) {
        pipe(pipes[pipe_index]);
    }

    pid_t pids[command_count];

    for (command_index = 0; command_index < command_count; command_index++) {
        pid_t pid;

        if ((pid = fork()) < 0) {
            printf("ERROR: forking child process failed\n");
            exit(1);
        }

        if (pid == 0) {
            if (command_index > 0) {
                dup2(pipes[command_index-1][0], STDIN_FILENO);
            }
            if (command_index + 1 < command_count) {
                dup2(pipes[command_index][1], STDOUT_FILENO);
            }

            close_all_pipes(pipes, command_count);

            if (execvp(*commands[command_index], commands[command_index])) {
                printf("ERROR: exec child process failed\n");
                exit(1);
            }
        } else {
            pids[command_index] = pid;
        }
    }

    close_all_pipes(pipes, command_count);

    int i = 0;
    for (i = 0; i < command_count; i++) {
        waitpid(pids[i], NULL, WUNTRACED);
    }
}

int find_pipe(char **tokens, int start_index) {

    int i = start_index;
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

        if ((size_t) i + 1 >= bufsize) {
            tokens = (char **)realloc(tokens, sizeof(char *) * ++bufsize);
        }
        tokens[i] = (char *)malloc(sizeof(char) * (strlen(token)+1));
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
    char ***result = (char ***) malloc(sizeof(char **) * bufsize);

    int command_id = 0;
    int curr_index = 0;
    int next_pipe_index = 0;
    while (next_pipe_index >= 0) {
        if ((size_t) command_id >= bufsize) {
            result = (char ***) realloc(result, sizeof(char **) * ++bufsize);
        }
        next_pipe_index = find_pipe(tokens, curr_index);
        char **command = NULL;
        if (next_pipe_index != -1) {
            command = (char **) malloc(sizeof(char *) * (next_pipe_index - curr_index));
            tokens[next_pipe_index] = NULL;
        } else {
            command = (char **) malloc(sizeof(char *) * (token_len - curr_index + 1));
        }
        command = tokens + curr_index;
        result[command_id++] = command;
        curr_index = next_pipe_index + 1;
    }
        
    return result;
}

void launch(char ***commands) {
    execute_command_loop(commands);
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
        line[strlen(line)-1] = '\0';
        tokens = tokenize(line);
        char ***commands = split_commands(tokens);
        launch(commands);
        printf("> ");
    }
    return;
}

