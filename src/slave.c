#include <unistd.h>
// #include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <poll.h>

#define MD5_COMMAND "/sbin/md5 "            // PATH AL MD5SUM en MacOS M3 (Nico)
// #define MD5_COMMAND "/sbin/md5sum" // PATH AL MD5SUM en MacOS M1
// #define MD5_COMMAND "/usr/bin/md5sum "   // PATH AL MD5SUM en Docker Linux
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define BUFFER_SIZE 1024
#define COMMAND_SIZE 2048

int main(int argc, char *argv[]) {
    char file[BUFFER_SIZE];
    pid_t pid = getpid();
    fsync(STDIN_FILENO);
    ssize_t buffer_read;

    while ((buffer_read = read(STDIN_FILENO, file, BUFFER_SIZE * sizeof(char))) >= 0)
    {

        char md5sum_command[COMMAND_SIZE];
        sprintf(md5sum_command, "%s \'%s\'", MD5_COMMAND, file);

        FILE *fp = popen(md5sum_command, "r");
        if (fp == NULL)
        {
            perror("popen");
            exit(EXIT_FAILURE);
        }

        char md5Buffer[BUFFER_SIZE];

        while (fgets(md5Buffer, sizeof(md5Buffer), fp) != NULL)
        {
            char md5[BUFFER_SIZE];
            sscanf(md5Buffer, "%s ", md5);
            fprintf(stdout, "File: %s - MD5: %s - PID: %d\n", file, md5, pid);
        }

        sleep(2);
        pclose(fp);
        exit(0);
    }

    return 0;
}

/*sem_t semaphore;

sem_init(&semaphore, 0, 1);  // Initialize semaphore with a value of 1
sem_wait(&semaphore);        // Decrease (wait)
sem_post(&semaphore);        // Increase (signal)
sem_destroy(&semaphore);     // Clean up*/
