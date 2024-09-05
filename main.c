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

    // main.c debe recibir los md5 de los slave.c y agregarlo al shm por orden de llegada
    // esto para se hace para el proceso view

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

    close(pipefd[READ_FD]);
    close(pipefd[WRITE_FD]);

    int status = 0;
    while((pid = wait(&status)) > 0);

    return 0;
}