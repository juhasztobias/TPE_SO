#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1
#define SHM_NAME "/md5_shared_memory"
#define SHM_SIZE 4096

int view() {
    // Esto deberia ir luego de haber abierto la shm
    /*
    while (LOOP) {
        // espera al main/master
        esperar_resultado( ... );  puede ser con down o wait(data_available);
        leer_resultado_de_shm( ... );
        imprimir_resultado_en_stdout( ... );
    }
    */

   // abro la shm del main.c para leer todo el contenido 

    /*
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }


    // aca leemos y mostramos en pantalla el contenido de la shm
    // el buffer deberia contener: nombre del file, md5 y PID del slave que lo proceso


    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME);
    */
}