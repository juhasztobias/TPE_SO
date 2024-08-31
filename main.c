#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <poll.h>

#define COMMANDS_BIN "/src/bin/"
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define SLAVES 5

int main(int argc, char * argv[]) {
    pid_t pid;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    system("whereis md5sum");
    for (int i = 0; i < SLAVES; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
            // close(pipefd[1]);
            close(0);
            dup(pipefd[0]);
            close(pipefd[0]);
            close(pipefd[1]);

            char * slave_o = "../src/bin/slave.o";
            char * slave_argv[] = {slave_o, NULL};
            execve(slave_o, slave_argv, NULL);

            perror("execve");
            exit(EXIT_FAILURE);
        }
    }
    
    fsync(1);

    struct pollfd pfd;
    pfd.fd = pipefd[READ_FD];
    pfd.events = POLLIN;
    for(int i = 1; i < argc; i++) {
        int ret = poll(&pfd, 1, 1);
        if (ret == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        if (ret == 0) {
            write(pipefd[WRITE_FD], argv[i], strlen(argv[i]) + 1); 
            continue;
        }

        i--;
    }

    close(pipefd[READ_FD]);
    close(pipefd[WRITE_FD]);

    int status = 0;
    while((pid = wait(&status)) > 0);

    return 0;
}