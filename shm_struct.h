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

void throwError(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}


#endif // SHM_STRUCT_H