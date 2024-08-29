#include <unistd.h>
// #include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>


#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]){
    // char slaveBuffer[1024];

    // // En el primero aparece el nombre del programa
    // for(int i = 1; i < argc; i++){
    //     sprintf(slaveBuffer, "md5sum \'%s\'", argv[i]);
    //     system(slaveBuffer);
    // }

    // return 0;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    char readBuffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[READ_FD], readBuffer, sizeof(readBuffer))) > 0) {
        sprintf(readBuffer, "md5sum \'%s\'", readBuffer);
        system(readBuffer);
    }
    
    close(pipefd[READ_FD]);
    close(pipefd[WRITE_FD]);

    return 0;
}