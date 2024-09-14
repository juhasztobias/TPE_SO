#include "shm_struct.h"
#include <errno.h>

#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define SHM_SIZE 4096
#define SHM_PTR_SIZE 8
#define SHM_NAME "/shm_md5"

void throwError(char *msg);

int main(int argc, char *argv[])
{
    struct shmbuf *shm_ptr;
    sleep(1);
    printf("Vista\n");
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1)
        throwError("shm_open");
    shm_ptr = mmap(NULL, sizeof(struct shmbuf), PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
        throwError("mmap");

    if (sem_post(&shm_ptr->sem_2) == -1)
        throwError("sem_post-2");
    do
    {
        while (sem_wait(&shm_ptr->sem_1) == -1){
            if (errno != EINTR)
                throwError("sem_wait-1");
        }
        printf("%s", shm_ptr->buffer);
        if (sem_post(&shm_ptr->sem_2) == -1)
            throwError("sem_post-2");
    } while (shm_ptr->buffer_size != 0);

    // Unmap and close shared memory
    munmap(shm_ptr, sizeof(struct shmbuf));
    close(shm_fd);
}

void throwError(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}