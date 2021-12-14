#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/types.h>


/**********************************
Gestion des fils de message
**********************************/
int msgid_1, msg_rep_id_1;

typedef struct {
    long type;
    pid_t pidEmetteur;
    int destination;
    int type_vehicule;
}msg_PlaceDispo;

typedef struct {
    long type;
    pid_t pidEmetteur;
    int bool_ok;
}rep_PlaceDispo;

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
void displayVehiculeInfo(int type, int destination, int nb_conteneur, int statut, int suite)
{
   //Type de vehicule
   if(type==1 && destination==1)
   {
   	  printf("Camion");
   }
   
   //Nombre conteneur
   printf(" %d (%d conteneur(s)| ",getpid(),nb_conteneur);
   
   //Destination
   if(destination==1){
   	  printf("Belfort");
   }
   
   printf(" | ");
   
   //Statut
   if(statut==-1){
   	  printf("en attente place à quai   ");
   }
   else if(statut==0){
   	  printf("en attente de dechargement");
   }
   else if(statut==1){
   	  printf("en attente de chargement  ");
   }else{
   	  printf("                          ");
   }
   
   if(suite==0){
   	  printf(")\n");
   }else{
      printf(") : ");
   }   
}

void vehicule(int type, int destination, int nb_conteneur)
{ 
  int quai, contenance_virtuelle;
  msg_PlaceDispo msgToSend;
  rep_PlaceDispo msgRep;

  //Définition des données propres véhicule
  quai=define_quai(destination);
  displayVehiculeInfo(type,destination,nb_conteneur,-1,0);
  
  /*Se garer*/
  P(quai-1,define_semid_type(type));
  if(nb_conteneur!=0){
    displayVehiculeInfo(type,destination,nb_conteneur,0,0);
  }

  //Vider le véhicule
  while(nb_conteneur!=0)                              
  {
    P(destination-1,sem_id_dest);
    displayVehiculeInfo(type,destination,nb_conteneur,999,1);
    printf("Je décharge un conteneur\n");
    nb_conteneur-=1;
  }

  //Definir les variables pour repartir
  destination=destination;
  contenance_virtuelle=1;
  displayVehiculeInfo(type,destination,nb_conteneur,999,1);
  printf("Ma nouvelle destination est %d\n",destination);

  //Remplir le véhicule
  while(contenance_virtuelle!=0)                      
  {
    //Construction du message place disponible
    msgToSend.type=getppid();
		msgToSend.pidEmetteur=getpid();
    msgToSend.destination=destination;
    msgToSend.type_vehicule=type;
    msgsnd(msgid_1, &msgToSend, sizeof(msg_PlaceDispo) - 4,0);
	
	displayVehiculeInfo(type,destination,nb_conteneur,1,0);
	//si la réponse reçue est 1 alors on va charger un conteneur, sinon non
    contenance_virtuelle-=1;
    msgrcv(msg_rep_id_1, &msgRep, sizeof(rep_PlaceDispo) - 4, 1,getpid());
    if(msgRep.bool_ok==1)
    {
      nb_conteneur+=1;
      displayVehiculeInfo(type,destination,nb_conteneur,999,1);
      printf("Je charge un conteneur\n");
    }
  }

  //Partir
  V(quai-1,define_semid_type(type));
  displayVehiculeInfo(type,destination,nb_conteneur,999,1);
  printf("Je m'en vais\n");
  exit(1);
}

/**********************************
Fonction de fonctionnement d'un portique
**********************************/
void portique(int quai)
{
  int destination,sem_value;
  msg_PlaceDispo msgRecu;
  rep_PlaceDispo msgRep;
 
  msgRep.pidEmetteur=getpid();
  
  while(1)
  {
    if(msgrcv(msgid_1, &msgRecu, sizeof(msg_PlaceDispo) - 4, 1,getpid())!=-1)
    {
		destination=msgRecu.destination;
		
		sem_value=semctl(sem_id_dest,destination-1,GETVAL);
		printf("Portique j'ai reçu un message de %d\n",msgRecu.pidEmetteur);
		
		if(sem_value<0){
			msgRep.bool_ok=1;
			msgRep.type=msgRecu.pidEmetteur;
			V(destination-1,sem_id_dest);
			msgsnd(msg_rep_id_1, &msgRep, sizeof(rep_PlaceDispo) - 4,0);
		}
		else{
			msgRep.bool_ok=0;
			msgRep.type=msgRecu.pidEmetteur;
			msgsnd(msg_rep_id_1, &msgRep, sizeof(rep_PlaceDispo) - 4,0);
		}
	 }
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
  msg_rep_id_1 = msgget(SKEY, 0750 | IPC_CREAT | IPC_EXCL);


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
