#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>


/**********************************
Gestion des fils de message
**********************************/
int msgid_1;

typedef struct {
    long type;
    pid_t pidEmetteur;
    int destination;
    int type_vehicule;
}msg_PlaceDispo;

/**********************************
Gestion des semaphores
**********************************/
#define IFLAGS (SEMPERM | IPC_CREAT)
#define SKEY   (key_t) IPC_PRIVATE	
#define SEMPERM 0600	
struct sembuf sem_oper_V ; 
int sem_id_camion,sem_id_dest ;

int initsem_parking(key_t semkey) 
{
  int status = 0;		
  int semid_init;
    union semun {
    int val;
    struct semid_ds *stat;
    ushort * array;
  } ctl_arg;
    if ((semid_init = semget(semkey, 2, IFLAGS)) > 0) {
        ushort array[2] = {0, 0};
        ctl_arg.array = array;
        status = semctl(semid_init, 0, SETALL, ctl_arg);
    }
    if (semid_init == -1 || status == -1) { 
  perror("Erreur initsem");
  return (-1);
    } else return (semid_init);
}
int initsem_destination(key_t semkey) 
{
  int status = 0;		
  int semid_init;
    union semun {
    int val;
    struct semid_ds *stat;
    ushort * array;
  } ctl_arg;
    if ((semid_init = semget(semkey, 2, IFLAGS)) > 0) {
        ushort array[10] = {0};
        ctl_arg.array = array;
        status = semctl(semid_init, 0, SETALL, ctl_arg);
    }
    if (semid_init == -1 || status == -1) { 
  perror("Erreur initsem");
  return (-1);
    } else return (semid_init);
}
 
void P(int semnum,int semid) {
  sem_oper_V.sem_num = semnum;
  sem_oper_V.sem_op  = -1 ;
  sem_oper_V.sem_flg  = 0 ;

  semop(semid,&sem_oper_V,1);
}

void V(int semnum,int semid) {
  sem_oper_V.sem_num = semnum;
  sem_oper_V.sem_op  = 1 ;
  sem_oper_V.sem_flg  = 0 ;

  semop(semid,&sem_oper_V,1);
}

/**********************************
Fonction qui retourne le sem_id en fonction du type de véhicule
**********************************/
int define_semid_type(int type)
{
  if(type==1){
    return sem_id_camion;
  }
}

/**********************************
Fonction qui retourne le numéro du quai concerné par une destination
**********************************/
int define_quai(int destination)
{
  return 1;
}

/**********************************
Fonction de fonctionnement d'un véhicule
**********************************/
void vehicule(int type, int destination, int nb_conteneur)
{ 
  int quai, contenance_virtuelle;
  msg_PlaceDispo msgToSend;

  //Définition des données propres véhicule
  quai=define_quai(destination);
  printf("Véhicule %d : Je cherche une place pour me garer\n",getpid());
  
  /*Se garer*/
  P(quai-1,define_semid_type(type));
  printf("Véhicule %d : Je me gare\n",getpid());
  
  //Vider le véhicule
  while(nb_conteneur!=0)                              
  {
    P(destination-1,sem_id_dest);
    printf("Véhicule %d : Je décharge un conteneur\n",getpid());
    nb_conteneur-=1;
  }

  //Definir les variables pour repartir
  destination=destination;
  contenance_virtuelle=1;
  printf("Véhicule %d : Ma nouvelle destination est %d\n",getpid(),destination);

  //Remplir le véhicule
  while(contenance_virtuelle!=0)                      
  {
    //Construction du message place disponible
    msgToSend.type=getppid();
		msgToSend.pidEmetteur=getpid();
    msgToSend.destination=destination;
    msgToSend.type_vehicule=type;
    if(msgsnd(msgid_1, &msgToSend, sizeof(msg_PlaceDispo) - 4,0)==-1){
      printf("message non envoyé\n");
    }

    contenance_virtuelle-=1;
    //if(read())
    //{
      nb_conteneur+=1;
      printf("Véhicule %d : Je charge un conteneur\n",getpid());
    //}
  }

  //Partir
  V(quai-1,define_semid_type(type));
  printf("Véhicule %d : Je m'en vais\n",getpid());
  exit(1);
}

/**********************************
Fonction de fonctionnement d'un portique
**********************************/
void portique(int quai)
{
  int destination;
  msg_PlaceDispo msgRecu;

  while(1)
  {
    if(msgrcv(msgid_1, &msgRecu, sizeof(msg_PlaceDispo) - 4, 1,getpid())!=-1){
      printf("Portique 1 : J'ai reçu un message avec destination : %d de la part de %d\n",msgRecu.destination,msgRecu.type);
    }    
    /*while()
    {
      read()
    }
    V(destination);
    write();*/
  }
}

/**********************************
Créé les véhicules et portiques
**********************************/
void gestionnaire_creation_vehicule()
{
  int status = 0;
  int i=0,pid,wpid;
  for(i=0;i<=4;i++)
  {
    pid=fork();
    if(pid==0){
      vehicule(1, 1, i%2);
    }
    else{
      
    }
  }
  while ((wpid = wait(&status)) > 0);
}

/**********************************
Main
**********************************/
int main(int argc, char *argv[])
{
  int pid_portique;

  //Creation du parking camion avec 2 places par quai
  sem_id_camion = initsem_parking(SKEY);
  V(0,sem_id_camion);
  V(0,sem_id_camion);
  V(1,sem_id_camion);
  V(1,sem_id_camion);


  //Creation de la semaphore des destinations
  sem_id_dest = initsem_destination(SKEY);

  //Creation file de message par quai
  msgid_1 = msgget(SKEY, 0750 | IPC_CREAT | IPC_EXCL);
  printf("%d\n",msgid_1);


  //Créations des portiques
  pid_portique=fork();
  if(pid_portique==0){
    portique(1);
  }
  else{
    //Creationdes véhicules
    gestionnaire_creation_vehicule();
  }
  
  return 0;
}
