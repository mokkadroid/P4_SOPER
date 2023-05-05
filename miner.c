#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pow.h"
#include "minero.h"


int main(int argc, char const *argv[]){
    unsigned int n, secs, mem;

    if(argc!=3){
        printf("fallo en numero argumentos!\n");
        return -1;
    }
    n = atoi(argv[1]);/* rondas */
    secs = atoi(argv[2]);/* tiempo de ejecucion en segundos */
    if(n<1|| secs<0){
        printf("Fallo en valor de argumentos!");
        return -1;
    }
     if((mem = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1){
        if(errno == EEXIST){
            /*Si ya se ha realizado y por tanto existe, lo añadimos
            y lo convertimos en el monitor de la ejecución*/
           if((mem = shm_open(SHM_NAME, O_RDWR, 0)) == -1){
                /*Control de errores sobre la memoria compartida*/
                perror("shm_open");
                return -1;
            }
            /* ponemos trg a 1 para indicarle a minero que no es el primero en conectarse al sistema */
            minero(1, 5, secs, mem);
            close(mem);
            return 0;
       }else{
            /*Mostramos si ha habido errores y salimos*/
            perror("shm_open");
            shm_unlink(SHM_NAME);
         
            return -1;
        }
    } 
    
    minero(0, 5, secs, mem);
    close(mem);
    return 0;
}
