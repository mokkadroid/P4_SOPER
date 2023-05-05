
#ifndef _WALLET_H
#define _WALLET_H


typedef struct {
    long int pid; /* pid del minero */
    int coins; /* numero de monedas */
    int active; /*Indica si el minero esta activo*/
}Wallet;

#endif