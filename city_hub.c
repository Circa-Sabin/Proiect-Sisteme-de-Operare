#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void start_monitor() {

    char buffer[BUFFER_SIZE];
    pid_t hub_pid;
    if ((hub_pid = fork()) < 0) {
        perror("Error at the fork\n");
        exit(-1);
    }
    else if (hub_pid == 0) {
        int pfd[2];
        pid_t monitor_pid;

        if ((pipe(pfd)) < 0) {
            perror("Error creating pipe\n");
            exit(-1);
        }

        if ((monitor_pid = fork()) < 0) {
            perror("Error forking\n");
            exit(-1);
        }

        else if (monitor_pid == 0) {
            close(pfd[0]);
            dup2(pfd[1],buffer);
            execlp("monitor_reports", "monitor_reports", NULL);
            perror("Eroare: nu a rulat cu succes\n");
            exit(EXIT_SUCCESS);
        }

        close(pfd[1]);
        read(pfd[0], buffer, BUFFER_SIZE);
        int n;
        while ((n = read(pfd[0], buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[n] = '\0';
            printf("([MONITOR]) %s])", buffer);
        }
        close(pfd[0]);
    }
}
