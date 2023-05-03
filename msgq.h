#include <mqueue.h>
#include "minero.h"

#define MQ_NAME "/mq_minero"

/**
 * Estructura para cada bloque de la cola
*/
typedef struct {
    int id; /* ID del bloque */
    long int sol;   /** Solución del bloque buffer*/
    long int obj;   /* Objetivo del bloque*/
    int trueSol;    /* Flag, que comprueba si la solución es válida*/
    int pid; /* PID del minero ganador */
    Wallet *wlt; /*wallets*/
} Bloque;
/* sol=-2 -> Codigo de error */