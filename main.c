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
#define SHM_NAME "/md5_shared_memory"
#define SHM_SIZE 4096
#define TWO 2

int main(int argc, char *argv[])
{
    pid_t pid[SLAVES];
    int file_pipes[SLAVES][TWO]; // Pipes main -> slave
    int hash_pipes[SLAVES][TWO]; // Pipes slave -> main

    for (int i = 0; i < SLAVES; i++)
    {
        if (pipe(file_pipes[i]) == -1)
        {
            perror("file_pipe");
            exit(EXIT_FAILURE);
        }
        if (pipe(hash_pipes[i]) == -1)
        {
            perror("hash_pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < SLAVES; i++)
    {
        pid[i] = fork();
        if (pid[i] == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid[i] == 0)
        { // Child (slave)
            // close(STDIN_FILENO);  // Close write-end of file pipe

            // Redirect pipes to stdin and stdout for the child
            dup2(file_pipes[i][READ_FD], STDIN_FILENO); // Redirect file pipe read-end to stdin
            // dup2(hash_pipes[i][WRITE_FD], STDOUT_FILENO); // Redirect hash pipe write-end to stdout

            // Close the originals after dup
            close(file_pipes[i][READ_FD]);

            char *slave_o = "../src/bin/slave.o";
            char *slave_argv[] = {slave_o, NULL};
            execve(slave_o, slave_argv, NULL);

            perror("execve");
            exit(EXIT_FAILURE);
        }
    }

    // Armo las estrcturas
    struct pollfd pfd[SLAVES];
    for (int i = 0; i < SLAVES; i++)
    {
        pfd[i].fd = file_pipes[i][READ_FD];
        pfd[i].events = POLLIN;
    }

    int slave = 0, i = 1;
    while (i < argc)
    {
        slave++;
        slave = slave % SLAVES;
        int ret = poll(&(pfd[slave]), 1, 1);
        if (ret == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        if (ret == 0)
        {
            write(file_pipes[slave][WRITE_FD], argv[i], strlen(argv[i]) + 1);
            i++;
            continue;
        }
    }

    for (int i = 0; i < SLAVES; i++)
    {
        close(file_pipes[i][READ_FD]);
        close(hash_pipes[i][READ_FD]);
        close(file_pipes[i][WRITE_FD]);
        close(hash_pipes[i][WRITE_FD]);
    }

    // int status = 0;
    while ((waitpid(-1, NULL, 0)) > 0)
        printf("Esperando\n");
    return 0;
}