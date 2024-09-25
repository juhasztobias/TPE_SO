// Based on the code from https://man7.org/linux/man-pages/man3/shm_open.3.html
#ifndef SHM_STRUCT_H
#define SHM_STRUCT_H

#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/fcntl.h>

#define BUFFER_SIZE 2048

struct shmbuf
{
    sem_t sem_1;
    sem_t sem_2;

    char buffer[BUFFER_SIZE];
    size_t buffer_size;
};

int printSemaphoreValue(sem_t *sem) {
    int value;
    if (sem_getvalue(sem, &value) == -1) 
        perror("sem_getvalue");
    return value;
}

void watchSharedMemory(char * sh_name, char * zone){
    struct shmbuf *shm_ptr;
    int shm_fd = -1;
    while (shm_fd == -1)
    {
        shm_fd = shm_open(sh_name, O_RDWR, 0666);
        if (shm_fd == -1 && errno == ENOENT)
        {
            usleep(100000); // Espera 100ms antes de intentar de nuevo
        }
        else if (shm_fd == -1)
            throwError("shm_open");
    }

    shm_ptr = mmap(NULL, sizeof(struct shmbuf), PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
        throwError("mmap");

    fprintf(stderr, "ZoneName:%s\t sem_1: %d\t sem_2: %d\t buff_size: %ld\n", zone,printSemaphoreValue(&shm_ptr->sem_1), printSemaphoreValue(&shm_ptr->sem_2), shm_ptr->buffer_size);
}

void throwError(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}


#endif // SHM_STRUCT_H