#include <unistd.h>
// #include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <poll.h>
#include <string.h>

#define MD5_COMMAND "/sbin/md5"            // PATH AL MD5SUM en MacOS M3 (Nico)
// #define MD5_COMMAND "/sbin/md5sum"      // PATH AL MD5SUM en MacOS M1
// #define MD5_COMMAND "/usr/bin/md5sum "   // PATH AL MD5SUM en Docker Linux
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define BUFFER_SIZE 1600
#define COMMAND_SIZE 2048

int main(int argc, char *argv[])
{
    char file[BUFFER_SIZE];
    pid_t pid = getpid();
    fsync(STDIN_FILENO);
    fsync(STDOUT_FILENO);
    ssize_t buffer_read;

    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    while (1)
    {
        int ret = poll(&pfd, 1, 1000); // Espera hasta 1 segundo por datos
        if (ret == 0)
            break; // Salir del bucle si no hay datos después de 1 segundo
        else if (ret == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        buffer_read = read(STDIN_FILENO, file, BUFFER_SIZE * sizeof(char));
        if (buffer_read <= 0)
            break;
        
        file[strcspn(file, "\n")] = 0; // Elimina el salto de línea del nombre del archivo si es que existe

        char md5sum_command[COMMAND_SIZE];
        //sprintf(md5sum_command, "%s \'%s\'", MD5_COMMAND, file);
        sprintf(md5sum_command, "%s %s", MD5_COMMAND, file);


        FILE *fp = popen(md5sum_command, "r");
        if (fp == NULL)
        {
            perror("popen");
            exit(EXIT_FAILURE);
        }

        char md5Buffer[BUFFER_SIZE];
        // char fullLine[BUFFER_SIZE * 2]; 

        while (fgets(md5Buffer, sizeof(md5Buffer), fp) != NULL)
        {
            char md5[BUFFER_SIZE];
            sscanf(md5Buffer, "%s ", md5);
            fprintf(stdout, "File: %s - MD5: %s - PID: %d\n", file, md5, pid);

            // // fullLine contiene el string completo 
            // snprintf(fullLine, sizeof(fullLine), "File: %s - MD5: %s - PID: %d\n", file, md5, pid);
            
            // size_t bytes = write(STDOUT_FILENO, fullLine, strlen(file));
            // if (bytes == -1) {
            // perror("write");
            // exit(EXIT_FAILURE);
            // }
        }

        pclose(fp);
    }

    fprintf(stdout, "Slave %d finished\n", pid);
    fflush(stdout);
    return 0;
}