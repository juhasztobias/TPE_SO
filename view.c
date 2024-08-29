#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1

int view() {
    /*
    while (LOOP) {
        esperar_resultado( ... );  puede ser con down o wait(data_available);
        leer_resultado_de_shm( ... );
        imprimir_resultado_en_stdout( ... );
    }
    */

}