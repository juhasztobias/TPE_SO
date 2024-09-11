#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <poll.h>
#include <sys/mman.h> 

// #define COMMANDS_BIN "/Users/tobiasjuhasz/Projects/ITBA/SO/TPE1/src/bin/"
#define COMMANDS_BIN "/Users/nicolaskim/Documents/SO/TP1/TPE_SO/src/bin/"
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define SLAVES 5
#define SHM_NAME "/md5_shared_memory"
#define SHM_SIZE 9096
#define TWO 2
#define BUFFER_SIZE 1600

int main(int argc, char *argv[])
{
    pid_t pid[SLAVES];
    int file_pipes[SLAVES][TWO];  // Pipes main -> slave
    int hash_pipes[SLAVES][TWO];  // Pipes slave -> main

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
            close(STDIN_FILENO);  // Close write-end of file pipe

            // Redirect pipes to stdin and stdout for the child
            dup2(file_pipes[i][READ_FD], STDIN_FILENO); // Redirect file pipe read-end to stdin
            //dup2(hash_pipes[i][WRITE_FD], STDOUT_FILENO); // Redirect hash pipe write-end to stdout

            // Close the originals after dup 
            close(file_pipes[i][READ_FD]);
            //close(hash_pipes[i][WRITE_FD]);

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
        slave_pid = waitpid(-1 , &status, WAIT_DEFAULT); //pid[i] por si queremos en orden
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

     /* [[[[  [[[[   creamos la shm   ]]]]  ]]]]*/
    
    // int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    // if (shm_fd == -1) {
    //     perror("shm_open");
    //     exit(EXIT_FAILURE);
    // }

    // if (ftruncate(shm_fd, SHM_SIZE) == -1) {
    //     perror("ftruncate");
    //     exit(EXIT_FAILURE);
    // }
    // void *shm_ptr = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0); // PROT_WRITE permite la escritura en la shm
    // if (shm_ptr == MAP_FAILED) {
    //     perror("mmap");
    //     exit(EXIT_FAILURE);
    // }

    // // Leer lo que los procesos slaves escriben en el hash pipe y escribirlo en la memoria compartida
    // char buffer[BUFFER_SIZE];
    // char *shm_write_ptr = (char *)shm_ptr;  // Puntero a la memoria compartida
    // for (int i = 0; i < SLAVES; i++)
    // {
    //     ssize_t bytes_read = read(hash_pipes[i][READ_FD], buffer, sizeof(buffer));
    //     if (bytes_read > 0)
    //     {
    //         // Escribir en la memoria compartida
    //         snprintf(shm_write_ptr, SHM_SIZE, "%s", buffer);
    //         shm_write_ptr += bytes_read;  // Mover el puntero
    //     }
    // }

    // // elimino y cierro todo lo de la shm
    // munmap(shm_ptr, SHM_SIZE);
    // close(shm_fd);
    // shm_unlink(SHM_NAME);
    

    // Ahora cerrar las tuberías de hash
    for (int i = 0; i < SLAVES; i++)
    {
        close(hash_pipes[i][READ_FD]);
        close(hash_pipes[i][WRITE_FD]);
    }
    return 0;
}