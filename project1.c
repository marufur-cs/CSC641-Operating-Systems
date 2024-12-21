#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 100

void sigint_handler(int sig) {
    printf("Ctrl+C is ignored\n");
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *argv[MAX_ARG_SIZE];
    pid_t pid;
    int status;

    signal(SIGINT, sigint_handler);

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        char *token = strtok(input, " ");
        int i = 0;
        int background = 0;


        while (token != NULL) {
            if (strcmp(token, "&") == 0) {
                background = 1; // Set background flag
                break; // Stop parsing further
            }
            argv[i++] = token;
            token = strtok(NULL, " ");
        }
        argv[i] = NULL;

        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            continue;
        } else if (pid == 0) {
            if (execvp(argv[0], argv) < 0) {
                perror("Execution failed");
                exit(EXIT_FAILURE);
            }
        } else {
            if (!background) {
                waitpid(pid, &status, 0);
            }
        }
    }

    return 0;
}
