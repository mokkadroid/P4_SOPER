#ifndef _MINERO_H
#define _MINERO_H

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include "msgq.h"
#include "wallet.h"

#define SHM_NAME "/shm_minersys" 


/**
 * Struct para sistema multiminero
*/
typedef struct {
    int onsys; /* mineros activos en el sistema */
    int votes[MAX_MINERS]; /* VOTOS DE CADA MINERO */
    int miners[MAX_MINERS]; /* array de PIDS de mineros activos*/
    Bloque last; /* bloque anterior resuelto */
    Bloque current; /*bloque actual*/
    int wlltfull; /* flag para indicar si la cartera esta llena */
    Wallet wllt[1000]; /* Carteras de los mineros */
    sem_t access; /* semaforo de acceso a la zona de memoria compartida */
    sem_t barrier; /* barrera para lectura y registro del bloque */
    sem_t mutex; /* mutex para controlar la barrera */
    int count; /* contador para la barrera */
} MinSys; 

/**
 * Función principal de minero
 * Se encarga de realizar las rondas de búsqueda por hash 
 * a partir de un objetivo e hilos definidos   
 * Input:                                                         
 * @param n: numero de hilos a usar por ronda             
 * @param trg: objetivo inicial de hash 
 * @param secs: tiempo que estara minando
*/

void minero(long int trg, int n, unsigned int secs, int fd);



#endif