#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <semaphore.h>

#define COMMANDS_BIN "/src/bin/"
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define SLAVES 5

// typedef int semaphore;
// extern semaphore data_available = 0;

int main(int argc, char * argv[])
{
    pid_t pid;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    

    for(int i = 1; i < argc; i++) { 
        write(pipefd[WRITE_FD], argv[i], strlen(argv[i]) + 1); 
    }

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
            close(pipefd[1]);
            char * slave_argv[] = {"slave", NULL};

            execve("./src/slave", slave_argv, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        }
    }

    close(pipefd[READ_FD]);
    close(pipefd[WRITE_FD]);

    // proceso padre
    int status = 0;
    while((pid = wait(&status)) > 0)
    {
        printf("El proceso padre espero al proceso hijo slave con PID: %d\n", pid);
    }
    // pid_t child_pid = waitpid(-1, &status, WAIT_DEFAULT);
    // if (child_pid > 0)
    //     printf("El proceso padre espero al proceso hijo slave con PID: %d\n", child_pid);
    // else
    // {
    //     perror("waitpid");
    //     exit(EXIT_FAILURE);
    // }

    return 0;
}