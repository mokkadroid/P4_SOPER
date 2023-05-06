#include <mqueue.h>
#include "wallet.h"

#define MAX_MINERS 100
#define MQ_NAME "/mq_minero"

/**
 * Estructura para cada bloque de la cola
*/
typedef struct {
    int id; /* ID del bloque */
    long int sol;   /** Solución del bloque buffer*/
    long int obj;   /* Objetivo del bloque*/
    int votes[2]; /* [0]-> a favor | [1]-> totales*/
    int pid; /* PID del minero ganador */
    Wallet wllt[MAX_MINERS];
} Bloque;
/* sol=-2 -> Codigo de error */
