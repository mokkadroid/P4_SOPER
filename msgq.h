#include <mqueue.h>
#include "minero.h"
#include "wallet.h"

#define MQ_NAME "/mq_minero"
#define MAX_MINER 100

/**
 * Estructura para cada bloque de la cola
*/
typedef struct {
    int id; /* ID del bloque */
    long int sol;   /** Solución del bloque buffer*/
    long int obj;   /* Objetivo del bloque*/
    int votes[2]; /* [0]-> a favor | [1]-> totales*/
    int pid; /* PID del minero ganador */
    Wallet wlt[MAX_MINER]; /*wallets*/
} Bloque;
/* sol=-2 -> Codigo de error */