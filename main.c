#include <poll.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include "shm_struct.h"

// #define SLAVE_BIN "/Users/tobiasjuhasz/Projects/ITBA/SO/TPE1/src/bin/slave.o"
// #define SLAVE_BIN "/Users/nicolaskim/Documents/SO/TP1/TPE_SO/src/bin/"
#define SLAVE_BIN "/app/src/bin/slave.o" // Docker
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define TWO 2
#define SHM_NAME "/shm_md5"

pid_t runSlave(char *slaveCommand, int file_pipes[TWO], int hash_pipes[TWO]);

int slavePipes(int* file_pipe, int* hash_pipe);
void closeSlavePipes(int *file_pipe, int *hash_pipe);
int writeSlavePipe(int *pipe, char *str);
int readSlavePipe(int *pipe, char *buffer);
int checkAvailability(int fd, int events, int timeout);
void closeMainPipes(int *file_pipe, int type);

void throwError(char *msg);

int createSharedMemory();
struct shmbuf *mapSharedMemory(int shm_fd);
void writeToSharedMemory(struct shmbuf *shm_ptr, char *buffer, size_t buffer_size);
void unMapSharedMemory(struct shmbuf *shm_ptr, int shm_fd);

int waitView(struct shmbuf *shm_ptr, int timeout_s);
int isDirectory(const char *path); // Function to check if a file is a directory

int main(int argc, char *argv[])
{
    int slaves = argc / 10 + 1;
    // int slaves = 10;
    int file_pipes[slaves][TWO]; // Pipes main -> slave
    int hash_pipes[slaves][TWO]; // Pipes slave -> main

    // Creamos el archivo resultado.txt
    FILE *result = fopen("resultado.txt", "w+");

    if (result == NULL) {
    perror("Error opening file");
    return EXIT_FAILURE;
}
    // Crear los procesos esclavos, guardamos los PIDs en un Array,
    // en file_pipes y hash_pipes los pipes de comunicación
    for (int i = 0; i < slaves; i++)
        runSlave(SLAVE_BIN, file_pipes[i], hash_pipes[i]);

    int shm_fd = createSharedMemory();
    struct shmbuf *shm_ptr = mapSharedMemory(shm_fd);

    if (sem_init(&shm_ptr->sem_1, 1, 0) == -1)
        throwError("sem_init-1");
    if (sem_init(&shm_ptr->sem_2, 1, 0) == -1)
        throwError("sem_init-2");

    int view_ready = waitView(shm_ptr, 2);

    int slave = 0, i = 1;
    while (i < argc)
    {
        if (isDirectory(argv[i])) { //Skipeo los directorios
            i++;
            continue;
        }

        slave++;
        slave = slave % slaves;

        i += writeSlavePipe(file_pipes[slave], argv[i]);

        char buffer[BUFFER_SIZE];
        int readBytes = readSlavePipe(hash_pipes[slave], buffer);
        if (readBytes == 0)
            continue;

        // Escribo en un archivo resultado.txt
        fwrite(buffer, sizeof(char), readBytes, result);
        if (view_ready)
            writeToSharedMemory(shm_ptr, buffer, readBytes);
    }

    // Esperar a que los procesos esclavos terminen
    int status, slave_pid, slaves_left = slaves;
    while ((slave_pid = waitpid(-1, &status, WAIT_DEFAULT)) > 0)
        slaves_left--;

    // Si no terminaron todos los slaves, tirar error
    if (slave_pid == -1 && slaves_left != 0)
        throwError("waitpid");

    if (view_ready)
        writeToSharedMemory(shm_ptr, "\0", 0);

    sem_destroy(&shm_ptr->sem_1);
    sem_destroy(&shm_ptr->sem_2);
    // // Unmap and close shared memory
    unMapSharedMemory(shm_ptr, shm_fd);

    // Cerramos el archivo resultado.txt
    fclose(result);

    // Cerramos los pipes de los procesos esclavos antes de terminar
    for (int j = 0; j < slaves; j++)
        closeSlavePipes(file_pipes[j], hash_pipes[j]);

    return 0;
}

/**
 * PIPES
 */

/**
 * Crea los pipes para la comunicación entre el main y los slaves
 */
int slavePipes(int* file_pipe, int* hash_pipe)
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
    if (checkAvailability(pipe[READ_FD], POLLIN, 10) != 0)
        return 0;
    write(pipe[WRITE_FD], str, strlen(str) + 1);
    return 1;
}

/**
 * Lee de un pipe
 */
int readSlavePipe(int *pipe, char *buffer)
{
    int ret = checkAvailability(pipe[READ_FD], POLLIN, 10);
    if (ret == 0)
        return 0;
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

        // close all other file descriptors
        closeMainPipes(file_pipes, WRITE_FD);
        closeMainPipes(hash_pipes, READ_FD);

        char *slave_argv[] = {slaveCommand, NULL};
        execve(slaveCommand, slave_argv, NULL);
        perror("execve");
        exit(EXIT_FAILURE);
    }
    return pid;
}

// Cierro los pipes que quedan abiertos en el main
void closeMainPipes(int *file_pipe, int type)
{
    // FIX: Arroja Bad file descriptor
    int value = fcntl(file_pipe[type], F_SETFD, FD_CLOEXEC);
    if (value == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
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
    int ret = poll(&pfd, 1, timeout);
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
    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd == -1)
        throwError("shm_open");
    if (ftruncate(shm_fd, sizeof(struct shmbuf)) == -1)
        throwError("ftruncate");
    return shm_fd;
}

void writeToSharedMemory(struct shmbuf *shm_ptr, char *buffer, size_t buffer_size)
{

    while (sem_wait(&shm_ptr->sem_2) == -1)
    {
        if (errno != EINTR)
            throwError("sem_wait-1");
    }
    if (sem_post(&shm_ptr->sem_1) == -1)
        throwError("sem_post-1");
    // throwError("sem_wait-2");
    shm_ptr->buffer_size = buffer_size;
    memcpy(shm_ptr->buffer, buffer, buffer_size);
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

int waitView(struct shmbuf *shm_ptr, int timeout_s)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        throwError("clock_gettime");
    ts.tv_sec += timeout_s;

    // int ret;
    // while((ret = sem_timedwait(&shm_ptr->sem_2, &ts)) == -1 && errno == EINTR);
    // if (ret == -1 && errno == ETIMEDOUT)
    //     return 0;
    // if (ret == -1)
    //     throwError("sem_timedwait");

    // Esto es pasar por pipe a view
    return 1;

    // No pasar por pipe a view
    // return 0;
}

// Check if the file is a directory
int isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0; // Cannot access path
    }
    return S_ISDIR(statbuf.st_mode);
}