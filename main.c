#include <poll.h>

#include "shm_struct.h"

// #define SLAVE_BIN "/Users/tobiasjuhasz/Projects/ITBA/SO/TPE1/src/bin/slave.o"
// #define SLAVE_BIN "/Users/nicolaskim/Documents/SO/TP1/TPE_SO/src/bin/"
#define SLAVE_BIN "/app/src/bin/slave.o"
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define TWO 2

pid_t runSlave(char *slaveCommand, int file_pipes[TWO], int hash_pipes[TWO]);

int slavePipes(int *filePipe, int *hashPipe);
void closeSlavePipes(int *file_pipe, int *hash_pipe);
int writeSlavePipe(int *pipe, char *str);
int readSlavePipe(int *pipe, char *buffer);
int checkAvailability(int fd, int events, int timeout);

void throwError(char *msg);

int createSharedMemory();
struct shmbuf *mapSharedMemory(int shm_fd);
void unMapSharedMemory(struct shmbuf *shm_ptr, int shm_fd);

int main(int argc, char *argv[])
{
    int slaves = argc / 10 + 1;
    int file_pipes[slaves][TWO]; // Pipes main -> slave
    int hash_pipes[slaves][TWO]; // Pipes slave -> main

    // Crear los procesos esclavos, guardamos los PIDs en un Array,
    // en file_pipes y hash_pipes los pipes de comunicación
    for (int i = 0; i < slaves; i++)
        runSlave(SLAVE_BIN, file_pipes[i], hash_pipes[i]);

    // Esperar dos segundos a que un proceso vista esté listo para leer de la memoria compartida
    sleep(2);
    int shm_fd = createSharedMemory();
    struct shmbuf *shm_ptr = mapSharedMemory(shm_fd);

    // Esperar dos segundos a que un proceso vista esté listo para leer de la memoria compartida
    // El proceso vista le indica READY al main
    // Esto se hace con un semaforo

    int slave = 0, i = 1;
    while (i < argc)
    {
        slave++;
        slave = slave % slaves;

        if (writeSlavePipe(file_pipes[slave], argv[i]) == 0)
        {
            char buffer[BUFFER_SIZE];
            int readBytes = readSlavePipe(hash_pipes[slave], buffer);
            if (readBytes != 0)
            {
                // Espero a que el proceso vista esté listo para leer
                if (sem_wait(&shm_ptr->sem_2) == -1)
                    throwError("sem_wait-2");
                shm_ptr->buffer_size = readBytes;
                snprintf(shm_ptr->buffer, readBytes + 1, "%s", buffer);

                // Indicar que ya se escribió en la memoria compartida
                if (sem_post(&shm_ptr->sem_1) == -1)
                    throwError("sem_post-1");
            }
            continue;
        }
        i++;
    }

    // Esperar a que los procesos esclavos terminen
    int status, slave_pid, slaves_left = slaves;
    while ((slave_pid = waitpid(-1, &status, WAIT_DEFAULT)) > 0)
        slaves_left--;

    // Si no terminaron todos los slaves, tirar error
    if (slave_pid == -1 && slaves_left != 0)
        throwError("waitpid");

    if (sem_wait(&shm_ptr->sem_2) == -1)
        throwError("sem_wait-2");

    // Le indico que terminé de escribir
    shm_ptr->buffer_size = 0;
    shm_ptr->buffer[0] = '\0';
    if (sem_post(&shm_ptr->sem_1) == -1)
        throwError("sem_post-1");
    sem_close(&shm_ptr->sem_1);
    sem_close(&shm_ptr->sem_2);
    // Unmap and close shared memory
    unMapSharedMemory(shm_ptr, shm_fd);

    // Cerramos los pipes de los procesos esclavos antes de terminar
    for (int i = 0; i < slaves; i++)
        closeSlavePipes(file_pipes[i], hash_pipes[i]);
    return 0;
}

/**
 * PIPES
 */

/**
 * Crea los pipes para la comunicación entre el main y los slaves
 */
int slavePipes(int *file_pipe, int *hash_pipe)
{
    return pipe(file_pipe) == -1 || pipe(hash_pipe) == -1 ? -1 : 0;
}

/**
 * Cierra los pipes de los procesos esclavos
 */
void closeSlavePipes(int *file_pipe, int *hash_pipe)
{
    close(file_pipe[WRITE_FD]);
    close(file_pipe[READ_FD]);
    close(hash_pipe[WRITE_FD]);
    close(hash_pipe[READ_FD]);
}

/**
 * Escribe en un pipe
 */
int writeSlavePipe(int *pipe, char *str)
{
    if (checkAvailability(pipe[READ_FD], POLLIN, 1) != 0)
        return 0;
    write(pipe[WRITE_FD], str, strlen(str) + 1);
    return 1;
}

/**
 * Lee de un pipe
 */
int readSlavePipe(int *pipe, char *buffer)
{
    if (checkAvailability(pipe[READ_FD], POLLIN, 100) != 0)
        return 0;
    printf("Reading from pipe\n");
    ssize_t bytes_read = read(pipe[READ_FD], buffer, BUFFER_SIZE * sizeof(char));
    if (bytes_read == -1)
        throwError("readSlavePipe");
    return bytes_read;
}
/**
 * TERMINA PIPES
 */

/**
 * PROCESOS
 */

/**
 * Ejecutar un proceso esclavo
 */
pid_t runSlave(char *slaveCommand, int file_pipes[TWO], int hash_pipes[TWO])
{
    // Crear los pipes para la comunicación entre el main y los slaves
    if (slavePipes(file_pipes, hash_pipes) == -1)
        throwError("Slave Pipe");

    pid_t pid = fork();
    if (pid == -1)
        throwError("Run slave fork");
    if (pid == 0)
    {
        close(STDIN_FILENO);  // Close write-end of file pipe
        close(STDOUT_FILENO); // Close read-end of hash pipe

        // Redirect pipes to stdin and stdout for the child
        dup2(file_pipes[READ_FD], STDIN_FILENO);   // Redirect file pipe read-end to stdin
        dup2(hash_pipes[WRITE_FD], STDOUT_FILENO); // Redirect hash pipe write-end to stdout

        // Close the originals after dup
        close(file_pipes[READ_FD]);
        close(hash_pipes[WRITE_FD]);

        char *slave_argv[] = {slaveCommand, NULL};
        execve(slaveCommand, slave_argv, NULL);
        perror("execve");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/**
 * TERMINA PROCESOS
 */

/**
 * UTILS
 */
/**
 * Lanza un error y termina el programa
 */
void throwError(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Checkea si un file descriptor tiene eventos disponibles
 */
int checkAvailability(int fd, int events, int timeout)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    int ret = poll(&pfd, 1, 100);
    if (ret == -1)
        throwError("poll");
    return ret;
}
/**
 * TERMINA UTILS
 */

// #region SHM
/**
 * Crea la memoria compartida
 */
int createSharedMemory()
{
    // Si ya existe la memoria compartida, la eliminamos
    // shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
        throwError("shm_open");
    if (ftruncate(shm_fd, sizeof(struct shmbuf)) == -1)
        throwError("ftruncate");
    return shm_fd;
}

/**
 * Mapea la memoria compartida
 */
struct shmbuf *mapSharedMemory(int shm_fd)
{
    struct shmbuf *shm_ptr = mmap(NULL, sizeof(struct shmbuf), PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
        throwError("mmap");
    return shm_ptr;
}

/**
 * Desmapea y cierra la memoria compartida
 */
void unMapSharedMemory(struct shmbuf *shm_ptr, int shm_fd)
{
    munmap(shm_ptr, sizeof(struct shmbuf));
    close(shm_fd);
    shm_unlink(SHM_NAME);
}
// #endregion