
#include "minero.h"
#include "pow.h"



#ifndef FALSE
  #define FALSE -1 /* FALSO */
  #define TRUE (!(FALSE)) /* VERDADERO */
#endif

int found = FALSE;
volatile sig_atomic_t usr1 = 0, usr2 = 0, cand = 0, alrm = 0, sint = 0; 


/* struct para los datos de trabajo de t_work */
typedef struct {
    long int obj; /* objetivo */
    long int sol; /* solucion a objetivo : pow_hash(sol) = obj */
    long int min; /* limite inferior del espacio de busqueda */
    long int max; /* limite superior del espacio de busqueda */
    
} Result; 

/************** Funciones privadas **************/ 
/* Para Result*/
Result* res_ini(long int trg, long int inf, long int sup);
void new_trg_set(Result** res, int n, long int trg);
void res_free(Result** res, int n);

/* Para los hilos */
void* t_work(void* args);
void join_check(int st);

/* Funciones auxiliares de minero */
long int round(Result** res, int n, MinSys* s);
long int* divider(int nthr);
int validator(MinSys* s);
int minero_map(MinSys** s, int fd, int og);
void minero_logoff(MinSys* s);

/*para shm y bloque*/
int wallet_set(Wallet* w, int miner, int flag);
int wallet_addminer(int* stat, Wallet* w, int id, int add);
int coins_add(Wallet* w, int cns);
int minsys_roundclr(MinSys* s);

/* proceso registrador */
void registrador(Bloque* b);

/******************** Codigo ********************/

/* Handlers para las señales */
void handler_usr1(int sig){
    usr1 = 1;
}
void handler_usr2(int sig){
    usr2 = 1;
}
void handler_int(int sig){
    sint = 1;
} 
void handler_alrm(int sig){
    alrm = 1;
}


/**
*  Funcion: res_ini                                
*                                                  
*  @brief Inicializa struct para poder trabajar con los argumentos necesarios de entrada y 
*  retorno de la funcion del hilo 
*                             
*  Input:                                          
*  @param trg: objetivo para resolver el hash             
*  @param inf: cota inferior del espacio de busqueda      
*  @param sup: cota superior del espacio de busqueda      
*  Output:                                         
*  Puntero struct de tipo Result                   
*/
Result* res_ini(long int trg, long int inf, long int sup){
    Result* res = NULL;
    /* Control de errores de los argumentos*/
    if(trg < 0 || inf >= sup || inf < 0 || sup < 0){
        printf("res_ini: fallo en argumentos\n");
        return NULL;
    }
    /* Reserva de memoria de la estructura*/
    res = (Result*) malloc(sizeof(Result));
    if(!res){
        perror("res_ini: ");
        return NULL;
    }
    /*Inicializa los campos de la estructura*/
    res->obj = trg;
    res->min = inf;
    res->max = sup;
    res->sol = -1; /*inicializamos sol como no encontrada*/

    return res;
} 


/**
*  Funcion: res_free                               
*                                                  
*  @brief Libera array de structs Result                  
*  Input:                                          
*  @param res: array a liberar                            
*  @param n: tamaño del array                             
*  Output:                                         
*  void                                            
*/
void res_free(Result** res, int n){
    int i = 0;
    if(!res) return;
    while (i < n){    /* Recorre el array completo para liberar recursos*/
        free(res[i]);
        i++;
    }
    free(res);
    
}


/**
*  Funcion: new_trg_set                            
*                                                  
*  @brief Actualiza objetivo de hash de un array de Results                                        
*  Input:                                          
*  @param res: array a actualizar                         
*  @param n: tamaño del array                             
*  @param trg: nuevo objetivo de hash del array           
*  Output:                                         
*  void                                            
*/
void new_trg_set(Result** res, int n, long int trg){
    int i = 0;

    /* Control de errores de los argumentos de entrada*/
    if(!res || trg < 0 || n < 0){
        printf("new_trg_set: Fallo en parametros\n");
        return;
    }
    
    /* Init de valores para actualizar el objetivo de los campos de la estructura*/
    while (i < n){
        res[i]->sol = -1;
        res[i]->obj = trg;
        i++;
    }
    found = FALSE; /*reset de found*/
}


/**
*  Funcion: t_work                                 
*                                                  
*  Trabajo asignado para cada hilo de minado en minero para la busqueda de hash   
*               
*  Input:                                          
*  @param args: puntero a struct con los datos            
*  Output:                                         
*  void                                            
*/
void* t_work(void* args){
    long int i, res = 0;
    i = ((Result *) args)->min;
    /*printf("Im on it\n");*/

    if (!args){
        printf("t_work: fallo en argumentos\n");
        return NULL;
    }
    if (found == TRUE){
        /*printf("key already found\n");*/
        return NULL;
    }

    /*Comprobamos si se encuentra una solución*/
    while(i <= ((Result *) args)->max && found == FALSE){
        res = pow_hash(i);
        if(res == ((Result *) args)->obj){
            ((Result *) args)->sol = i;/*la solucion al hash es i*/
            found = TRUE;
            /*printf("key found\n");*/
          /*una vez encontrada la clave, salimos*/
            return NULL;
        }
        i++;
    }
    return NULL;
}


/**
*  Funcion: round                                  
*                                                  
*  @brief Crea los hilos que hacen la busqueda del hash y manda la solucion como nuevo target    
*       
*  Input:                                          
*  @param res: puntero a array de datos para cada hilo    
*  @param n: numero de hilos                              
*  Output:                                         
*  El nuevo target                                 
*/
long int round(Result** res, int n, MinSys* s){
    pthread_t* pth = NULL;
    int i = 0, j = 0, st, count = 0, win = 0; /*st-> status de pthread_create | win-> flag para saber si el retorno sera > 0 */
    long int new_trg = -1; /* solo sera >0 si encuentra la solucion */

    if(!res || n < 1 || !s){
        printf("Round: fallo en argumentos\n");
        return new_trg;
    }

    /* Reserva de memoria en la creación de los hilos*/
    pth = (pthread_t*) malloc(n * sizeof(pthread_t));
    if (!pth){
        perror("round --> malloc pth: ");
        return new_trg;
    }
    

    /* creamos los hilos mientras que no se haya recibido la señal */
    for ( i = 0; i < n && usr2<1; i++){
        st = pthread_create(&pth[i], NULL, t_work, (res[i]));
        count++;
        /*printf("hilo creado: %d\n", st);*/
        if(st != 0){
            printf("Round --> pthread_create: %d\n", st);
            free(pth);
            return new_trg;
        }
    }
    /* Comprobamos la ejecucion */
    i = 0;
    for (i = 0; i < count; i++){
        st=pthread_join(pth[i], NULL);
        if (st != 0){
            printf("join number: %d\n", i);
            free(pth);
            join_check(st);
            return new_trg;
        }
        if (res[i]->sol > 0 && usr2 < 1){
            new_trg = res[i]->sol;
            for (i = 0; i < MAX_MINERS; i++){
                if(s->miners[i] > -1){
                    kill(s->miners[i], SIGUSR2);
                    j++;
                }
                if(j == s->onsys) break;
            }
            /* accedemos a memoria para actualizar el bloque que se acaba de resolver */
            sem_wait(&(s->access));
            if (s->b.sol<0){ /* Comprobamos que sea el primer minero en acceder */
                syst->b.sol = new_trg;             
                syst->b.pid = getpid();
              /* Obtenemos las carteras de los mineros activos en este momento*/
                j=0;
                for (i = 0; i < 1000; i++){
                    if (syst->wllt[i].active == 1){
                        syst->b.wllt[j].active = syst->wllt[i].active;
                        syst->b.wllt[j].coins = syst->wllt[i].coins;
                        syst->b.wllt[j].pid = syst->wllt[i].pid;
                        j++;
                    }
                    if (j == syst->onsys) break;
                }
               
                win = 1;
                printf("Solucion de ronda: %08ld\n", new_trg);
            }
            sem_post(&(s->access));
        } 
        
    }
    usr2 = 0; /* reseteamos la flag de usr2 para poder indicar luego el comienzo de la votacion */
    if(win == 0) new_trg = -2;
    if(count > 0) free(pth);
    return new_trg;
}


/**
*  Funcion: divider                                
*                                                  
*  Crea los intervalos para los espacios de busqueda para cada hilo                         
* 
*  Input:                                          
*  @param nthr: numero de hilos entre los que dividir el espacio de busqueda                    
*  Output:                                         
*  array con los limites de cada intervalo         
*/
long int* divider(int nthr){
   long int* minmax = NULL;
   int i = 2, a = 1;
   double quant = 0.00, b = nthr;

    /* Control de errores y reserva de memoria para los intervalos*/
    if(nthr < 1) return NULL;
    minmax = (long int*) malloc((2 * nthr) * sizeof(long int));
    if(!minmax){
        perror("Fallo en divider: ");
        return NULL;
    }

    /* los espacios van delimitados por a/N : a={2, 3, ..., N} */
    /* como cota inferior tendran ((a-1)/N)+1 */
    quant = a / b;
    minmax[0] = 0;
    minmax[1] = POW_LIMIT * quant;

    while(i < (2 * nthr)){
        quant = a / b;
        if(i % 2 == 0){/* los indices pares son los minimos */
            minmax[i] = (POW_LIMIT * quant) + 1;
            a++;
        } else { /* los impares los maximos */
            minmax[i] = POW_LIMIT * quant;
        }
        i++;
    }

    return minmax;
}


int wallet_set(Wallet* w, int miner, int flag){
    if (!w || miner < 0){
        printf("fallo en argumentos\n");
        return -1;
    }
    
    w->active = flag;
    w->coins = 0;
    w->pid = miner;
    return 0;
    
}


int coins_add(Wallet* w, int cns){
    if(!w || cns<1){
        printf("coins_add->Fallo en argumentos\n");
        return -1;
    }

    w->coins += cns;
    return 0;
}


/**
*  Funcion: wallet_addminer
*  
*  @brief Añade un minero al array de carteras del sistema
*  
*  @param stat: indica si el array está lleno o no, cambia tras la adicion de la nueva cartera, 
*               poner a -1 para eliminar la cartera de un bloque o NULL para añadirla en este.
*  @param w: array de carteras en donde se añade la del nuevo minero.
*  @param id: pid del minero.
*  @param add: 1 si se quiere añadir cartera, 0 si el minero se ha desconectado 
*               (se sobreescribira si es necesario).
*  
*  Output:
*  0 si todo ok, -1 si hay error en parametros o no se ha podido añadir la cartera
*/
int wallet_addminer(int* stat, Wallet* w, int id, int add){
    int i = 0;

    if(!w || id < 0){
        printf("wallet_addminer -> fallo en argumentos\n");
        return -1;
    }

    if(add == 0 && (*stat) >= 0){
        for (i = 0; i < 1000; i++){
            if (id == w[i].pid){
                w[i].active = 0;
                return 0;
            }
        }
    } else if(add == 0 && (*stat) == -1){
        for (i = 0; i < MAX_MINERS; i++){
            if (id == w[i].pid){
                w[i].active = -1;
                w[i].coins = 0;
                w[i].pid = -1;
                return 0;
            }
        }
    } else if (add == 1 && !stat){
        for (i = 0; i < MAX_MINERS; i++){
            if(w[i].pid<0){
                w[i].pid = id;
                w[i].active = 1;
                w[i].coins = 0;
                return 0;  
            }
        }
        
    } else if (add == 1 && stat){
        for (i = 0; i < 1000; i++){
            if ((*stat) == 1000 && w[i].active == 0){
                w[i].pid = id;
                w[i].active = 1;
                w[i].coins = 0;
                return 0;
            }
            if (w[i].active < 0){ /* solo sera <0 si el hueco esta libre */
                w[i].pid = id;
                w[i].active = 1;
                w[i].coins = 0;
                (*stat)++;
                return 0;
            }
        }
    }
    return -1;
}
/**
 * Funcion: minsys_roundclr
 * 
 * @brief Prepara el sistema para la ronda siguiente: transfiere el ultimo bloque minado a
 * last y deja current con el array de carteras vacio y el nuevo objetivo de minado
 * 
 * @param syst: puntero al sistema de mineros
 * 
 * Output:
 * 0 si todo OK, -1 en caso de error
*/
int minsys_roundclr(MinSys* s){
 int i=0;

   if(!syst){
        printf("fallo en minsys_roundclr: argumentos");
        return -1;
   }
   syst->last.id = syst->b.id;
   syst->last.pid = syst->b.pid;
   syst->last.obj = syst->b.obj;
   syst->last.sol = syst->b.sol;
   syst->last.votes[0] = syst->b.votes[0];
   syst->b.votes[0] = 0;
   syst->last.votes[1] = syst->b.votes[1];
   syst->b.votes[1] = 0;
   for (i = 0; i < MAX_MINERS; i++){
        if (syst->last.wllt[i].active == 1){
            syst->last.wllt[i].active = syst->b.wllt[i].active;
            syst->last.wllt[i].coins = syst->b.wllt[i].coins;
            syst->last.wllt[i].pid = syst->b.wllt[i].pid;
            syst->b.wllt[i].active = -1; /* se "borra" la cartera ya copiada */
        }    
   }
   syst->b.obj = syst->last.sol;
   syst->b.id++;
   syst->b.sol=-1;
   
   return 0; 
}  

/**
*  Funcion: minero_map                                
*                                                  
*  Se encarga de mapear el segmento de memoria compartida y conectar el minero 
*          
*  Input:                                          
*  @param buf: puntero doble a buffer memoria compartida a inicializar o actualizar             
*  @param fd: descriptor de fichero del segmento de shm            
*  @param og: flag para indicar si es minero que abre el sistema (1) o si se une a el (0)                             
*  Output:                                         
*  void                                             
*/
int minero_map(MinSys** s, int fd, int og){
    int i = 0, st = 0;
    if(fd < 0 || og < 0){
        printf("minero_map: fallo en args\n");
        return -1;
    }
    /*Comprobamos si se asigna correctamente la sección de memoria a la variable buf*/
    if ((*s = mmap(NULL, sizeof(MinSys), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
        perror("mmap"); 
        return -1;
    }
    close(fd);

    if(og){
        sem_init(&((*s)->access), 1, 1); /* inicializamos primero el semaforo */
        sem_wait(&((*s)->access)); /*bloqueamos el acceso a los datos*/
        
        (*s)->onsys = 1;
        (*s)->wlltfull = 0;
        (*s)->b.obj = 0;
        (*s)->b.sol = -1;
        (*s)->b.id = 0;/*id del bloque actual por resolver */
        (*s)->b.votes[0] = 0; /*votos a favor*/
        (*s)->b.votes[1] = 0;/*votos totales*/
        /*inicializamos el array de mineros activos*/
        (*s)->miners[0] = getpid();
        (*s)->votes[0] = -1;
        for (i = 1; i < MAX_MINERS; i++){ 
            (*s)->miners[i] = -1;
            (*s)->votes[i] = -1;
        }

        if ((st = wallet_set(&((*s)->wllt[0]), (*s)->miners[0], 1)) < 0){
            printf("minero_map: fallo en wallet_set primer minero\n");
            return -1;
        }
        (*s)->wlltfull++;
        for (i = 1; i < (MAX_MINERS * 10); i++){
            if ((st = wallet_set(&((*s)->wllt[i]), 0, -1)) < 0){
                    printf("minero_map: fallo en wallet_set\n");
                    return -1; 
            }
        }
        sem_post(&((*s)->access));
    } else {
        if((st = sem_wait(&((*s)->access))) < 0){
            perror("El sistema esta siendo liberado o ha ocurrido lo siguiente: ");
            return -1;
        }
        for (i = 0; i < MAX_MINERS; i++){ 
            if ((*s)->miners[i] == -1){
                (*s)->miners[i] = getpid();
                (*s)->onsys++;
                st = 0;
                break;
            }
            st = 1;
        }
        /* Si se ha podido unir al sistema, se crea una cartera */
        if (st == 1){
            if((st = wallet_addminer(&((*s)->wlltfull), (*s)->wllt, getpid(), 1)) < 0){
               sem_post(&((*s)->access));
               printf("minero_map: fallo en wallet_addminer\n");
               return -1; 
            }
        }
        
        sem_post(&((*s)->access));
    }
    return st;
}


void minero_logoff(MinSys* s){
    int i = 0, miner, st = -1;

    if(!s) return;
    miner = getpid();
    sem_wait(&(s->access));
    s->onsys--;
    for (i = 0; i < MAX_MINERS; i++){
        if(s->miners[i] == miner) s->miners[i] = -1;
    }

    if((i = wallet_addminer(&st, s->b.wlt, miner, 0)) < 0){
        printf("minero_logoff: fallo en wallet_addminer\n");
        munmap(s, sizeof(s));
        return;
    }

    if(s->onsys == 0){
        sem_destroy(&(s->barrier));
        sem_destroy(&(s->mutex));
        sem_post(&(s->access));
        sem_destroy(&(s->access));
    }

    munmap(s, sizeof(s));
}


/**
*  Funcion: minero                                 
*                                                  
*  Realiza las rondas de minado por hash que se    
*  quieren hacer, dando un objetivo inicial        
*  Input:                                          
*  @param rnd: numero de rondas de minado                 
*  @param n: numero de hilos a usar por ronda             
*  @param trg: objetivo inicial de hash, indica si el minero es el primero a entrar en el sistema o no                
*  @param msecs: espera entre rondas en ms                
*  Output:                                         
*  void                                            
*/
void minero(long int trg, int n, unsigned int secs, int fd){
    int i = 0, j = 0, win = 0, st;
    long int* minmax = NULL, solucion;
    Result** res = NULL;
    MinSys* systmin = NULL;
    struct sigaction act1, act2, actint, actlrm;
    sigset_t set, oset;

    /* Control de errores sobre los parametros de entrada*/
    if (trg < 0 || n < 1 || fd < 0 || secs < 0){
        printf("Minero: Error en parametros\n");
        exit(EXIT_FAILURE);
    }
    /* bloqueamos las señales primero */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGALRM);   
    sigprocmask(SIG_BLOCK, &set, &oset);

    /*definimos los handlers*/
    /* Para SIGALRM */
    actlrm.sa_handler = handler_alrm;
    sigemptyset(&(actlrm.sa_mask));
    sigaddset(&(actlrm.sa_mask), SIGUSR1);
    sigaddset(&(actlrm.sa_mask), SIGUSR2);
    sigaddset(&(actlrm.sa_mask), SIGALRM); 
    sigaddset(&(actlrm.sa_mask), SIGINT);
    actlrm.sa_flags = 0;
    /* Preparamos la alarma lo primero */
    if (sigaction(SIGALRM, &actlrm, NULL) < 0) {
        perror("Sigaction");
        exit(EXIT_FAILURE);
    }
    if (alarm(secs)){
        printf ("Minero: Hay una alarma anterior.\n");
        exit(EXIT_FAILURE);
    }

    /* Para SIGINT */
    actint.sa_handler = handler_int;
    sigemptyset(&(actint.sa_mask));
    sigaddset(&(actint.sa_mask), SIGUSR1);
    sigaddset(&(actint.sa_mask), SIGUSR2);
    sigaddset(&(actint.sa_mask), SIGALRM); 
    actint.sa_flags = 0;

    /* Para SIGUSR1 */
    act1.sa_handler = handler_usr1;
    sigemptyset(&(act1.sa_mask));
    sigaddset(&(act1.sa_mask), SIGUSR1);
    sigaddset(&(act1.sa_mask), SIGUSR2);
    sigaddset(&(act1.sa_mask), SIGALRM); 
    sigaddset(&(act1.sa_mask), SIGINT);
    act1.sa_flags = 0;
    /* Para SIGUSR2 */
    act2.sa_handler = handler_usr2;
    sigemptyset(&(act2.sa_mask));
    sigaddset(&(act2.sa_mask), SIGUSR1);
    sigaddset(&(act2.sa_mask), SIGUSR2);
    sigaddset(&(act2.sa_mask), SIGALRM); 
    sigaddset(&(act2.sa_mask), SIGINT);
    act2.sa_flags = 0;

    if (sigaction(SIGINT, &actint, NULL) < 0) {
        perror("Sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR1, &act1, NULL) < 0) {
        perror("Sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR2, &act2, NULL) < 0) {
        perror("Sigaction");
        exit(EXIT_FAILURE);
    }
    /* Desbloquemos señales despues de la preparacion de los handlers */
    sigprocmask(SIG_UNBLOCK, &set, &oset);
    
    /* abrimos shm */
    if(trg == 0){
        if (ftruncate(fd, sizeof(MinSys)) == -1){
            /*Mostramos mensajes de error y perror si ha habido 
            algún error durante la llamada a ftruncate*/
            printf("Error al reservar la memoria compartida.\n");
            perror("Ftruncate");
            /*Cerramos la memoria*/
            close(fd);
            exit(EXIT_FAILURE);
        }
        if((st = minero_map(&systmin, fd, 1)) < 0){
            printf("minero: Error de minero_map\n");
            munmap(systmin, sizeof(MinSys));
            exit(EXIT_FAILURE);
        } 
        win++;

    } else {
        if((st = minero_map(&systmin, fd, 0)) < 0){
            printf("minero: Error de minero_map\n");
            munmap(systmin, sizeof(MinSys));
            exit(EXIT_FAILURE);
        }
    }
    /* Llamada a la función auxiliar de minero*/ 
    minmax = divider(n);
    if(!minmax){
        printf("Minero: Error en minmax\n");
        minero_logoff(systmin);
        exit(EXIT_FAILURE);
    }

    /* Ahora creamos el array de Result */
    res = (Result**) malloc(n * sizeof(Result*));
    if (res==NULL){
        perror("Minero: Error en res, ");
        minero_logoff(systmin);
        free(minmax);
        exit(EXIT_FAILURE);
    }
     
    if(trg > 0) trg = systmin->b.obj; /* Si no es el primer minero en conectarse al sistema, 
                                 * se pone como objetivo de busqueda lo que haya en shm */
    for (i = 0; i < n; i++){
        res[i] = res_ini(trg, minmax[j], minmax[j + 1]);
        if (res[i] == NULL){
            printf("Minero: Error en bucle de res\n");
            i--;
            while (i >= 0) {
                free(res[i]);
                i--;
            }
            minero_logoff(systmin);
            free(res);
            free(minmax);
            exit(EXIT_FAILURE);
        }
        j += 2;
    }

    /* Bucle de ejecucion de las rondas */
    while (alrm < 1 || sint < 1){
        /* Si es el ganador o el primer minero, manda USR1 para iniciar minado */
        if(win == 1){
            sem_wait(&(systmin->access));
            sem_init(&(systmin->barrier), 1, 0);
            sem_init(&(systmin->mutex), 1, 1);
            systmin->count = 0;
            sem_post(&(systmin->access));
            j = 0;
            for (i = 0; i < MAX_MINERS; i++){
                if(systmin->miners[i] > -1){
                    kill(systmin->miners[i], SIGUSR1);
                    j++;
                }
                if(j == systmin->onsys) break;
            }
            win = 0; 
        }
        /* Esperamos USR1 para comenzar ronda de minado */
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        sigprocmask (SIG_BLOCK, &set, NULL);
        while (usr1 == 0) sigsuspend (&oset);
        sigprocmask (SIG_UNBLOCK, &set, NULL);
        usr1 = 0; /*reseteamos la flag de USR1*/
        solucion=round(res,n, systmin);
        if(solucion == -1){  
            printf("Fallo en ronda\n");
            /*mq_close(q);*/
            res_free(res, n);
            free(minmax);
            exit(EXIT_FAILURE);
        } else if (solucion >= 0){
            win = 1; /*indicamos que es el ganador de la ronda para la proxima iteracion */
            /* mandamos señal para comenzar a votar */
            j = 0;
            for (i = 0; i < MAX_MINERS; i++){
                if(systmin->miners[i] > -1){
                    kill(systmin->miners[i], SIGUSR2);
                    j++;
                }
                if(j == systmin->onsys) break;
            }
        } 
        /* Esperamos a USR2 para votar */
        sigemptyset(&set);
        sigaddset(&set, SIGUSR2);
        sigprocmask (SIG_BLOCK, &set, NULL);
        while (usr2 == 0) sigsuspend (&oset);
        sigprocmask (SIG_UNBLOCK, &set, NULL);
        usr2 = 0; /*reseteamos la flag de usr2 */
        /**
         *  comienza la votacion, el ganador tambien vota llamando
         *  a monitor para validar la solucion que ha obtenido
         */
        if((st = validator(systmin)) < 0){
            printf("minero: error en validador\n");
            minero_logoff(systmin);
            res_free(res, n);
            free(minmax);
            exit(EXIT_FAILURE);
        }
        /**
         * para asegurarnos que todos los los procesos han salido 
         * de la votacion, y no se registre mal el bloque en ningun
         * caso, se ha implementado una barrera:
        */
        sem_wait(&(systmin->mutex));
        systmin->count++;
        sem_post(&(systmin->mutex));
        /* ponemos >= por si hay algun minero que haya votado pero no se haya leido su voto*/
        if(systmin->count >= systmin->b.votes[1]) sem_post(&(systmin->barrier));
        sem_wait(&(systmin->barrier));
        sem_post(&(systmin->barrier));
        /* aqui mandariamos por tuberia el bloque a registrador */
        sem_post(&(systmin->access));
        if(win==1){
            printf("Bloque a registrar:\n");
            printf("Id: %04d\nWinner: %d\nTRG: %08ld\nSOL: %08ld\nVotes: %d/%d\n", systmin->b.id, systmin->b.pid, systmin->b.obj,
             systmin->b.sol, systmin->b.votes[0], systmin->b.votes[1]);
            
            st=minsys_roundclr(systmin);
            if (st<0){
                sem_post(&(systmin->access));
                minero_logoff(systmin);
                res_free(res, n);
                free(minmax);
                exit(EXIT_FAILURE);
            }
            
        }
        sem_post(&(systmin->access));
        new_trg_set(res, n, systmin->b.obj);
    }
    /* liberado de memoria */
    /*mq_close(q);*/
    minero_logoff(systmin);
    res_free(res, n);
    free(minmax);
    exit(EXIT_SUCCESS);
}


int validator(MinSys* s){
    int pid, i = 0, voted = 0;
    /* voted -> flag para que pueda emitir su voto el minero ganador una sola vez */
    long int solhash;
    if(!s){
        printf("validador: puntero a shm NULL\n");
        return -1;
    }
    solhash = pow_hash(s->b.sol);/* En solhash se guarda la solucion correcta al hash */

    if ((pid = getpid()) == s->b.pid){ /* Si es el minero ganador */
        for (i = 0; i < s->onsys; i++){ /* comprueba tantas veces como mineros conectados haya*/
            struct timespec rqtp, rmtp = {0, 250 * 1000000};
            nanosleep(&rqtp, &rmtp); /*espera inactiva de 250ms*/
            sem_wait(&(s->access));
            if(s->votes[i] != -1){
                s->b.votes[0] += s->votes[i];/*votos a favor*/
                s->b.votes[1]++;/*votos totales*/
            } else {
                if(voted == 0){ 
                    if(solhash == s->b.obj){
                        s->votes[i] = 1;
                    } else {
                        s->votes[i] = 0;
                    }
                    s->b.votes[0] += s->votes[i];/*votos a favor*/
                    s->b.votes[1]++;/*votos totales*/
                    voted++;/* se sube la flag para indicar que ya ha votado */
                }
            }
            sem_post(&(s->access));
        }
    } else {
        sem_wait(&(s->access));
        for (i = 0; i < MAX_MINERS; i++){
            if (s->votes[i] < 0){
                if(solhash == s->b.obj){
                    s->votes[i] = 1;
                } else {
                    s->votes[i] = 0;
                } 
                sem_post(&(s->access));
                return 0;
            }
        }
        sem_post(&(s->access));
        printf("[%d] no ha podido votar\n", getpid());
        return -1;
    }

    return 0;
}


void registrador(Bloque* b){
    if(!b){
        printf("error en registrador: bloque vacio\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}


/**
*  Funcion: join_check                             
*                                                  
*  Control de errores de pthread_join(), printea   
*  por pantalla el significado del codigo de error 
*  Input:                                          
*  @param st: codigo de error dado por pthread_join()     
*  Output:                                         
*  void                                            
*/
void join_check(int st){
    switch (st) {
        case EDEADLK:
            printf("Deadlock detected\n");
            break;
        case EINVAL:
            printf("The thread is not joinable\n");
            break;
        case ESRCH:
            printf("No thread with given ID is found\n");
            break;
        default:
            printf("Error occurred when joining the thread\n");
    }

}
