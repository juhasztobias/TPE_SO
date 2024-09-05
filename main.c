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
    int file_pipes[SLAVES][TWO];  // Pipes main -> slave
    int hash_pipes[SLAVES][TWO];  // Pipes slave -> main

    for (int i = 0; i < SLAVES; i++)
    {
        if (pipe(file_pipes[i]) == -1) {
            perror("file_pipe");
            exit(EXIT_FAILURE);
        }
        if (pipe(hash_pipes[i]) == -1) {
            perror("hash_pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < SLAVES; i++)
    {
        pid[i] = fork();
        if (pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid[i] == 0) { // Child (slave)
            close(file_pipes[i][WRITE_FD]);  // Close write-end of file pipe
            // close(hash_pipes[i][READ_FD]);   // Close read-end of hash pipe

            // Redirect pipes to stdin and stdout for the child
            dup2(file_pipes[i][READ_FD], STDIN_FILENO);   // Redirect file pipe read-end to stdin
            // dup2(hash_pipes[i][WRITE_FD], STDOUT_FILENO); // Redirect hash pipe write-end to stdout

            // Close the originals after dup
            close(file_pipes[i][READ_FD]);
            // close(hash_pipes[i][WRITE_FD]);

            char *slave_o = "../src/bin/slave.o";
            char *slave_argv[] = {slave_o, NULL};
            execve(slave_o, slave_argv, NULL);

            perror("execve");
            exit(EXIT_FAILURE);
        }
    }

    fsync(1);
    
    // Armo las estrcturas
    struct pollfd pfd[SLAVES];
    for(int i = 0; i < SLAVES; i++){
        pfd[i].fd = file_pipes[i][WRITE_FD]; //REVISAR
        pfd[i].events = POLLIN;
    }


    // // Máximo de 5 archivos por slave.
    // // Porque son 2 slaves y son 10 archivos
    int slave = 0;
    for(int i = 1; i < argc; i++, slave++){
        slave = slave % SLAVES;
        int ret = poll(&(pfd[slave]), 1, 1);
        if (ret == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        if(ret == 0){
            printf("slave: %d\n", slave);
            write(file_pipes[slave][WRITE_FD], argv[i], strlen(argv[i]) + 1);
            continue;
        }
        i--;
    }

    // main.c debe recibir los md5 de los slave.c, para imprimir por pantalla y agregarlo a la shm por orden de llegada

    // creamos la shm

    /*
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0); // PROT_WRITE permite la escritura en la shm
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // elimino y cierro todo lo de la shm
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME);
    */
   
    for(int i = 0; i < SLAVES; i++) {
        close(file_pipes[i][READ_FD]);
        close(hash_pipes[i][READ_FD]);
        close(file_pipes[i][WRITE_FD]);
        close(hash_pipes[i][WRITE_FD]);
    }
    printf("aca llega\n");

    int status = 0;
    for (int i = 0; i < SLAVES; i++) {
        while ((pid[i] = wait(&status)) > 0);
    }

    printf("aca llega\n");
    
    return 0;
}