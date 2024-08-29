#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <semaphore.h>

#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1

typedef int semaphore;
extern semaphore data_available = 0;

int main(void) {
    pid_t pid;
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        char *slave_argv[] = {"slave", NULL};
        execve("slave", slave_argv, NULL);
        perror("execve");
        exit(EXIT_FAILURE);
    }

    int status;
    pid_t child_pid = waitpid(-1, &status, WAIT_DEFAULT);
    if (child_pid > 0)
        printf("El proceso padre espero al proceso hijo slave con PID: %d\n", child_pid);
    else {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    /*
    while(LOOP) {
        esperar_resultado( ... );  // mejora la sync de los procesos
        publicar_resultado_en_shm( ... );
        // le avisa a view 
        avisar_resultado_disponible( ... );  // puede ser con up o post(data_available);
    }
    */

    return 0;
}