#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <poll.h>

#define COMMANDS_BIN "/Users/tobiasjuhasz/Projects/ITBA/SO/TPE1/src/bin/"
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

    char slave_o[SHM_SIZE];
    sprintf(slave_o, "%s%s", COMMANDS_BIN, "slave.o");
    for (int i = 0; i < SLAVES; i++)
    {
        pid[i] = fork();
        if (pid[i] == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid[i] == 0)
        {
            // close(STDIN_FILENO);  // Close write-end of file pipe

            // Redirect pipes to stdin and stdout for the child
            dup2(file_pipes[i][READ_FD], STDIN_FILENO); // Redirect file pipe read-end to stdin
            // dup2(hash_pipes[i][WRITE_FD], STDOUT_FILENO); // Redirect hash pipe write-end to stdout

            // Close the originals after dup (falta el de hash_pipes)
            close(file_pipes[i][READ_FD]);

            char *slave_argv[] = {slave_o, NULL};
            execve(slave_o, slave_argv, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        }
    }

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
        if (ret != 0)
            continue;

        write(file_pipes[slave][WRITE_FD], argv[i], strlen(argv[i]) + 1);
        i++;
    }

    // Cerrar solo las tuberías de archivo
    for (int i = 0; i < SLAVES; i++)
    {
        close(file_pipes[i][READ_FD]);
        close(file_pipes[i][WRITE_FD]);
    }

    // Después del bucle while que escribe los argumentos
    for (int i = 0; i < SLAVES; i++)
    {
        close(file_pipes[i][WRITE_FD]);
    }

    // Esperar a que los procesos esclavos terminen
    int status, slave_pid;
    for (int i = 0; i < SLAVES; i++)
    {
        slave_pid = waitpid(pid[i], &status, 0);
        if (slave_pid > 0)
        {
            printf("Main espero al proceso slave con PID: %d\n", slave_pid);
        }
        else
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }

    // Ahora cerrar las tuberías de hash
    for (int i = 0; i < SLAVES; i++)
    {
        close(hash_pipes[i][READ_FD]);
        close(hash_pipes[i][WRITE_FD]);
    }
}