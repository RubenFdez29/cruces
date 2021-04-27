#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys/wait.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include "cruce.h"

#define TAM 256
#define MAX 126

//SEMAFOROS 0 Y 1 SON LOS DE COCHES
//SEMAFOROS 2 Y 3 SON LOS DE PEATONES

int semaforo, memoriaID, num_procesos, buzon;
char *memoriaComp = NULL;
int PID_PADRE, controlador;
struct tipo_mensaje{
    long tipo;
    int mensaje;
};
struct tipo_mensaje msg1, msg2;

void handler_SIGINT(int signo){//Revisar esto en encina
    if(signo==SIGINT){
        if(getpid() == PID_PADRE){
            CRUCE_fin();
            int retorno, i;
        
            //Esperar a los hijos
            for(i=0;i<num_procesos;i++){
                wait(&retorno);
            }
            //Borrar semaforo
            if(semctl(semaforo, 0, IPC_RMID)==-1){
                printf("\nError semctl\n");
            }
            if(shmdt(memoriaComp)==-1){//Desasociamos
                printf("\nError desasociar memoria compartida\n");
            }
            //Borrar bloque de memoria
            if(shmctl(memoriaID, IPC_RMID, NULL)==-1){
                printf("\nError liberar memoria compartida\n");
            }
        }
    }
    exit(0);
}
//Funcion del siclo semaforico
void cicloSem(){

    int i;
    while(1){
        CRUCE_pon_semAforo(0, VERDE);
        CRUCE_pon_semAforo(1, ROJO);
        CRUCE_pon_semAforo(2, VERDE);
        CRUCE_pon_semAforo(3, ROJO);

        /*pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();*/

        for(i=0; i=5; i++){
            pausa();
        }

        CRUCE_pon_semAforo(0, ROJO);
        CRUCE_pon_semAforo(1, VERDE);
        CRUCE_pon_semAforo(2, ROJO);
        CRUCE_pon_semAforo(3, ROJO);

        /*pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();*/

        for(i=0; i=8; i++){
            pausa();
        }

        CRUCE_pon_semAforo(0, ROJO);
        CRUCE_pon_semAforo(1, ROJO);
        CRUCE_pon_semAforo(2, ROJO);
        CRUCE_pon_semAforo(3, VERDE);

        /*pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();
        pausa();*/

        for(i=0; i=11; i++){
            pausa();
        }
    }
}

/*pid_t crearHijo(){
    pid_t pid=fork();

    switch(pid){
        case -1:
            exit(0);
        break;
    }
    return pid;
}*/

//Funcion para mover coches o peatones
void movimiento(int rol){
    struct posiciOn possig;
    if(rol==0){
        possig=CRUCE_inicio_coche();
        while (possig.y>=0){
            printf("A %d : x:%d y:%d",getpid(),possig.x,possig.y);
            if((msgrcv(buzon, &msg1, sizeof(struct tipo_mensaje)-sizeof(long),(possig.x+3)*100+possig.y, 0))==-1){
                printf("Error al mandar mensaje");
            }
            possig=CRUCE_avanzar_coche(possig);
            pausa_coche();
            if(possig.y==10){
                if(possig.x>3){
                    msg1.tipo=(possig.x-4)*100+10;
                    if((msgsnd(buzon, &msg1, sizeof(struct tipo_mensaje)-sizeof(long), 0))==-1){
                        printf("Error al mandar mensaje");
                    } 
                }
            }
            if(possig.x==33){
                if(possig.y>4){
                    msg1.tipo=(possig.x-4)*100+10;
                    if((msgsnd(buzon, &msg1, sizeof(struct tipo_mensaje)-sizeof(long), 0))==-1){
                        printf("Error al mandar mensaje");
                    } 
                }
            }
        }
        CRUCE_fin_coche();
    }
    if(rol==1){
        
    }
}

void mapear(){
    struct tipo_mensaje msg1;
    int x, y;

    //Forma de saber que posiciones estan libres en la carretera
    //Horizontal tipos de 0110-3610 
    for(x=-3;x<=33;x+=2){
        msg1.tipo=(x+3)*100+10;
        if((msgsnd(buzon, &msg1, sizeof(struct tipo_mensaje)-sizeof(long), 0))==-1){
            printf("Error al mandar mensaje");
        }
    }

    //Vertical tipos de 3300-3320
    for(y=1;y<=20;y++){
        msg1.tipo=36*100+y;
        if((msgsnd(buzon, &msg1, sizeof(struct tipo_mensaje)-sizeof(long), 0))==-1){
            printf("Error al mandar mensaje");
        }
    }
}

int main (int argc, char *argv[]){
    PID_PADRE = getpid();
    int i, rol, velocidad;
    struct sigaction sigIntHandler;
    
    //Manejador ctrl+c
    /*if(signal(SIGINT, sig_action)==SIG_ERR){
    	printf("Error");
        return 2;
    }*/
    sigIntHandler.sa_handler=handler_SIGINT;
    sigIntHandler.sa_flags=0;
    sigemptyset(&sigIntHandler.sa_mask);
    if(sigaction(SIGINT,&sigIntHandler,NULL)==1){
        printf("Error manejador");
        return 2;
    }

    //Comprobacion de parametros
    if(argc<3 | argc>3){
        printf("Parametros insuficientes");
        return 1;
    }
    num_procesos=atoi(argv[1]);
    if(num_procesos<=0 | num_procesos>126 ){
        printf("Parametro número de procesos incorrecto");
        return 1;
    }
    velocidad=atoi(argv[2]);
    if(velocidad<0){
        printf("Parametro velocidad incorrecto");
        return 1;
    }

    //Crea memoria compartida
    if((memoriaID=shmget(IPC_PRIVATE, TAM, IPC_CREAT|0600))==-1){
        printf("Error al reservar memoria compartida");
        return 3;
    }
    memoriaComp=shmat(memoriaID,NULL,0);//Asociamos

    //Crea semaforo
    if((semaforo=semget(IPC_PRIVATE, 5, IPC_CREAT|0600))==-1){
        printf("Error al reservar semaforo");
        return 3;
    }

    //Crea buzon
    if((buzon=msgget(IPC_PRIVATE, IPC_CREAT|0600))==-1){
        printf("Error al reservar buzon");
        return 3;
    }

    //Mapeamos con mensajes la carretera
    mapear();
    CRUCE_inicio(velocidad,num_procesos,semaforo,memoriaComp);

    switch(fork()){
        case -1:
            printf("ERROR al crear el hijo");
        break;
        case 0:
            /*if(getpid() == controlador){
                cicloSem();
            }*/
        break;
        default:
            while(1){
                if(getpid()==PID_PADRE){
                    while((rol=CRUCE_nuevo_proceso())!=0){
                        rol=CRUCE_nuevo_proceso();
                    }
                    switch(fork()){
                        case -1:
                            printf("ERROR al crear el hijo");
                            break;
                        case 0:
                            movimiento(rol);
                            break;
                        default:
                            break;
                    }            
                }
            }
        break;
    }
    return 0;
}
