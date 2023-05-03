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

#define MAX_MINER 100
#define SHM_NAME "/shm_minersys" 


/**
 * Struct para sistema multiminero
*/
typedef struct {
    int onsys; /* mineros activos en el sistema */
    int nvotes; /* numero de votos */
    int votes[MAX_MINER]; /* VOTOS DE CADA MINERO */
    int miners[MAX_MINER]; /* array de PIDS de mineros activos*/
    int last; /* bloque anterior resuelto */
    int curr; /* bloque actual por resolver*/
    long int obj;/* objetivo a resolver */
    long int sol;/* solucion encontrada */
    int wlltfull; /* flag para indicar si la cartera esta llena */
    Wallet wllt[1000]; /* Carteras de los mineros */
    sem_t access; /* semaforo de acceso a la zona de memoria compartida */
} MinSys; 

typedef struct {
    long int pid; /* pid del minero */
    int coins; /* numero de monedas */
    int active; /*Indica si el minero esta activo*/
}Wallet;


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