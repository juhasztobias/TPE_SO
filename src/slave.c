#include <unistd.h>
// #include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <poll.h>
#include <string.h>

// #define MD5_COMMAND "/sbin/md5"            // PATH AL MD5SUM en MacOS M3 (Nico)
// #define MD5_COMMAND "/sbin/md5sum" // PATH AL MD5SUM en MacOS M1
#define MD5_COMMAND "/usr/bin/md5sum " // PATH AL MD5SUM en Docker Linux
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define BUFFER_SIZE 1600
#define COMMAND_SIZE 2048

char *getMD5(char *buff, char *fileName, pid_t pid);
int canRead();
void writePipe(char *str);

int main(int argc, char *argv[])
{
    pid_t pid = getpid();
    char file[BUFFER_SIZE];

    fsync(STDIN_FILENO);
    fsync(STDOUT_FILENO);

    while (1)
    {
        int readable = canRead();
        if (readable == 0)
            break;
        else if (readable == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        ssize_t buffer_read;
        buffer_read = read(STDIN_FILENO, file, BUFFER_SIZE * sizeof(char));
        if (buffer_read <= 0)
            break;

        // Elimina el salto de línea del nombre del archivo si es que existe
        file[strcspn(file, "\n")] = 0;
        char buffer[BUFFER_SIZE * 2];
        // if (strcmp(file, "main.o") == 0)
        //     continue;
        writePipe(getMD5(buffer, file, pid));
    }

    fflush(stdout);
    return 0;
}

char *getMD5(char *buff, char *fileName, pid_t pid)
{
    char md5sum_command[COMMAND_SIZE];
    sprintf(md5sum_command, "%s \'%s\'", MD5_COMMAND, fileName);
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
        sscanf(md5Buffer, "%1599s", md5);
        sprintf(buff, "File: %s - MD5: %s - PID: %d\n", fileName, md5, pid);
    }
    pclose(fp);

    return buff;
}

int canRead()
{
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    return poll(&pfd, 1, 1000); // Espera hasta 1 segundo por datos
}

void writePipe(char *str)
{
    // fprintf(stdout, "Writing: %s\n", str);
    struct pollfd pfd;
    pfd.fd = STDOUT_FILENO;
    pfd.events = POLLIN;

    int ret;
    while ((ret = poll(&(pfd), 1, 1)) != 0)
    {
        if (ret == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }
    }

    write(STDOUT_FILENO, str, strlen(str) + 1);
}