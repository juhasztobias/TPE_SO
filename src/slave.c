#include <unistd.h>
// #include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#define MD5_COMMAND "/sbin/md5sum "         // PATH AL MD5SUM en MacOS
// #define MD5_COMMAND "/usr/bin/md5sum "   // PATH AL MD5SUM en Docker Linux
#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define BUFFER_SIZE 1024
#define COMMAND_SIZE 256

int main(int argc, char *argv[]){
    char file[BUFFER_SIZE];
    pid_t pid = getpid();
    fsync(0);
    ssize_t buffer_read;
    while((buffer_read = read(STDIN_FILENO, file, BUFFER_SIZE * sizeof(char))) > 0) {
        char md5sum_command[COMMAND_SIZE];
        sprintf(md5sum_command, "%s \'%s\'", MD5_COMMAND,file);

        FILE * fp = popen(md5sum_command, "r");
        if (fp == NULL) {
            perror("popen");
            exit(EXIT_FAILURE);
        }

    // estamos imprimiendo en pantalla/terminal directamente desde el slave
    // tenemos que pasarle por los pipes al main el dato formateado para que el main imprima por pantalla e inserte en la shm 
    
        char md5Buffer[BUFFER_SIZE];
        while(fgets(md5Buffer, sizeof(md5Buffer), fp) != NULL) {
            char md5[BUFFER_SIZE];
            sscanf(md5Buffer, "%s ", md5);
            fprintf(stdout, "File: %s - MD5: %s - PID: %d\n", file, md5, pid);
        }
        // fprintf(stdout, "%d\n", pid);
        pclose(fp);
    }
    return 0;
}