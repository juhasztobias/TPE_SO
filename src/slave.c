#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#define READ_FD 0
#define WRITE_FD 1
#define WAIT_DEFAULT 0
#define LOOP 1

int main(){
    char* ejemplo="md5sum view.c";
    int i=system(ejemplo);
    printf("%d",i);
    return i;
}