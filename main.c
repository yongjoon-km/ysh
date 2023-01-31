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
                // dup2(fd2[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                if (execvp(command2, argv2) < 0) {
                    printf("ERROR: exec child process2 failed\n");
                    exit(1);
                }
            }
        } else {
            // dup2(fd2[0], STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);
            while (wait(&status) != pid);
        }
    }
}

void execute_command_loop(char ***commands) {
    int command_index = 0;

    for (command_index = 0; commands[command_index] != NULL; command_index++) {
        // TODO create child processes and connect to pipe
        pid_t pid;

        if ((pid = fork()) < 0) {
            printf("ERROR: forking child process failed\n");
            exit(1);
        }

        if (pid == 0) {
            if (execvp(*commands[command_index], commands[command_index])) {
                printf("ERROR: exec child process failed\n");
                exit(1);
            }
        } 
    }

    wait(NULL);
}

// TODO: Create general execute command function
void execute_command_recursive(char ***commands, int command_index, int in_fd, int out_fd) {

    if (commands[command_index] == NULL) {
        return;
    }

    pid_t pid;
    int fd[2];
    int status;
    bool has_next_command = commands[command_index + 1] != NULL;

    if (has_next_command && pipe(fd) < 0) {
        printf("ERROR: creating pipe faild\n");
        exit(1);
    }

    if ((pid = fork()) < 0) {
        printf("ERROR: forking child process%d failed\n", command_index);
        exit(1);
    }
    else if (pid == 0) {
        dup2(in_fd, STDIN_FILENO);
        if (has_next_command) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            close(fd[0]);
        } 
        if (execvp(*commands[command_index], commands[command_index]) < 0) {
            printf("ERROR: exec child process failed\n");
            exit(1);
        }
    }
    else {
        execute_command_recursive(commands, command_index + 1, fd[0], fd[1]);
        if (has_next_command) {
            close(fd[0]);
            close(fd[1]);
        }
        wait(NULL);
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

        if ((size_t) i + 1 >= bufsize) {
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
    char ***result = (char ***) malloc(sizeof(char **) * bufsize);

    int command_id = 0;
    int processing_command_start_index = 0;
    int pipe_char_index = 0;
    while (pipe_char_index >= 0) {
        if ((size_t) processing_command_start_index + 1 > bufsize) {
            result = (char ***) realloc(result, sizeof(char **) * ++bufsize);
        }
        pipe_char_index = find_pipe(tokens + processing_command_start_index);
        char **command = NULL;
        if (pipe_char_index != -1) {
            command = (char **) malloc(sizeof(char *) * (pipe_char_index - processing_command_start_index));
            tokens[pipe_char_index] = NULL;
        } else {
            command = (char **) malloc(sizeof(char *) * (token_len - processing_command_start_index));
        }
        command = tokens + processing_command_start_index;
        result[command_id++] = command;
        processing_command_start_index = pipe_char_index + 1;
    }
        
    return result;
}

void launch(char ***commands) {

    // TODO: Change to run arbitrary number of commands
    // execute_command(*commands[0], commands[0], *commands[1], commands[1]);
    // Failed the below approach, I think shell process should manange all the pipe file descriptors.
    // execute_command_recursive(commands, 0, STDIN_FILENO, STDOUT_FILENO);
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

