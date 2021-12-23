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
int msgid_1, msg_rep_id_1, msgid_2, msg_rep_id_2;
int temps_dechargement=2;
int temps_chargement=2;
int temps_deplassement_portique=2;
int temps_nettoyage=4;
int temps_garage=2;

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

int define_msg_quai(int quai){
  if(quai==1){return msgid_1;} 
  else if(quai==2){return msgid_2;}
} 

int define_msg_rep_quai(int quai){
  if(quai==1){return msg_rep_id_1;} 
  else if(quai==2){return msg_rep_id_2;}
} 

/**********************************
Gestion des semaphores
**********************************/
#define IFLAGS (SEMPERM | IPC_CREAT)
#define SKEY   (key_t) IPC_PRIVATE	
#define SEMPERM 0600	
struct sembuf sem_oper_V ; 
int sem_id_camion,sem_id_dest,sem_id_bateau,sem_id_train ;

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
    if ((semid_init = semget(semkey, 20, IFLAGS)) > 0) {
        ushort array[20] = {0};
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
  }else if(type==2){
    return sem_id_train;
  }else if(type==3){
    return sem_id_bateau;
  } 
}

/**********************************
Fonction qui retourne le numéro du quai concerné par une destination
**********************************/
int define_quai(int destination)
{
  if(destination<10)
 {
   return 1;
 } 
  else
  {
    return 2;
  }  
}

/**********************************
Fonction de fonctionnement d'un véhicule
**********************************/
int contenance_max(int type)
{
  if(type==1){return 1;}
  else if(type==2){return 6;}
  if(type==3){return 12;}
}

void displayVehiculeInfo(int type, int destination, int nb_conteneur, int statut, int suite)
{
  //Quai
  int quai=define_quai(destination);
  printf("Quai %d | ",quai);

   //Type de vehicule
   if(type==1)
   {
   	  printf("Camion");
   }else if(type==2){
      printf("Train ");
   }else if(type==3){
      printf("Bateau");
   } 
   
   //Nombre conteneur
   printf(" %d (%d conteneur(s)| ",getpid(),nb_conteneur);
   
   //Destination
   if(destination==1){
   	  printf("Belfort");
   }
   else if(destination==2){
     printf("Dijon  ");
   } 
   else
   {
      printf("Lyon   ");
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
  int quai, contenance_virtuelle, nb_decharge=0;
  msg_PlaceDispo msgToSend;
  rep_PlaceDispo msgRep;

  //Définition des données propres véhicule
  quai=define_quai(destination);
  displayVehiculeInfo(type,destination,nb_conteneur,-1,0);
  
  /*Se garer*/
  P(quai-1,define_semid_type(type));
  displayVehiculeInfo(type,destination,nb_conteneur,-1,1);
  printf("Je me gare\n");
  sleep(temps_garage);

  if(nb_conteneur!=0){
    displayVehiculeInfo(type,destination,nb_conteneur,0,0);
  }

  //Vider le véhicule
  while(nb_conteneur!=0)                              
  {  
    //sleep(1);
    //printf("demande dec avec sem_id=%d et sem_num=%d\n",sem_id_dest,destination-1);
    P(destination-1,sem_id_dest);

    displayVehiculeInfo(type,destination,nb_conteneur,999,1);
    printf("Je décharge un conteneur\n");
    sleep(temps_dechargement);
    nb_conteneur-=1;
    nb_decharge+=1;
  }

  //Definir les variables pour repartir
  sleep(1);
  destination=destination;
  contenance_virtuelle=contenance_max(type);
  //Si le véhicule viens de décharger des conteneurs on le passe en mode expedition
  if(nb_decharge!=0)
  {
    displayVehiculeInfo(type,destination,nb_conteneur,999,1);
    printf("Nettoyage du véhicule\n");
    sleep(temps_nettoyage);
    displayVehiculeInfo(type,destination,nb_conteneur,999,1);
    printf("Ma nouvelle destination est %d\n",destination);
  }  
  

  


  //Remplir le véhicule
  while(contenance_virtuelle!=0)                      
  {
    //Construction du message place disponible
    msgToSend.type=1;
		msgToSend.pidEmetteur=getpid();
    msgToSend.destination=destination;
    msgToSend.type_vehicule=type;
    msgsnd(define_msg_quai(define_quai(destination)), &msgToSend, sizeof(msg_PlaceDispo) - 4,0);
	
	  displayVehiculeInfo(type,destination,nb_conteneur,1,0);
	  //si la réponse reçue est 1 alors on va charger un conteneur, sinon non
    contenance_virtuelle-=1;
    msgrcv(define_msg_rep_quai(define_quai(destination)), &msgRep, sizeof(rep_PlaceDispo) - 4, 2,1);
    //printf("Reponse de la part du portique : %d\n",msgRep.bool_ok);

    if(msgRep.bool_ok==1)
    {
      nb_conteneur+=1;
      displayVehiculeInfo(type,destination,nb_conteneur,999,1);
      printf("Je charge un conteneur\n");
    }
    sleep(temps_chargement);
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
    if(msgrcv(define_msg_quai(quai), &msgRecu, sizeof(msg_PlaceDispo) - 4, 1,1)!=-1)
    {
      destination=msgRecu.destination;
      //printf("Portique %d j'ai reçu un message de %d je regarde si on a une demande de dechargement\n",quai,msgRecu.pidEmetteur);
      //sleep(.5);

      sem_value=semctl(sem_id_dest,destination-1,GETNCNT);
      //printf("sem_value=%d avec sem_id=%d et sem_num=%d\n",sem_value,sem_id_dest,destination-1);
      
      if(sem_value>0){
        msgRep.bool_ok=1;
        msgRep.type=2;
        V(destination-1,sem_id_dest);
        sleep(temps_dechargement);
        sleep(temps_deplassement_portique);
        msgsnd(define_msg_rep_quai(quai), &msgRep, sizeof(rep_PlaceDispo) - 4,0);
        sleep(temps_deplassement_portique);
      }
      else{
        msgRep.bool_ok=8;
        msgRep.type=2;
        msgsnd(define_msg_rep_quai(quai), &msgRep, sizeof(rep_PlaceDispo) - 4,0);
      }
	 }
  }
}

/**********************************
Créé les véhicules
**********************************/
int create_vehicule(int type, int destination, int nb_conteneur)
{
  int pid;
  pid=fork();
  if(pid==0){
    vehicule(type, destination, nb_conteneur);
  }
  else{
    sleep(.2);
    return 1;
  }  
}


void gestionnaire_creation_vehicule()
{
  int camion,bateau,train,belfort,dijon,lyon;

  camion=1;
  train=2;
  bateau=3;

  belfort=1;
  dijon=2;
  lyon=10;

  create_vehicule(camion,lyon,0);
  create_vehicule(train,lyon,4);
  create_vehicule(camion,lyon,0);
  create_vehicule(camion,lyon,0);
  sleep(20);
  create_vehicule(bateau,lyon,3);
  create_vehicule(camion,lyon,0);
}

/**********************************
Main
**********************************/
int main(int argc, char *argv[])
{
  int pid_portique1,pid_portique2,status;

  //Creation du parking camion avec 2 places par quai
  sem_id_camion = initsem_parking(SKEY);
  sem_id_bateau = initsem_parking(SKEY);
  sem_id_train = initsem_parking(SKEY);

  V(0,sem_id_camion);
  V(0,sem_id_camion);
  V(1,sem_id_camion);
  V(1,sem_id_camion);

  V(0,sem_id_bateau);
  V(1,sem_id_bateau);

  V(0,sem_id_train);
  V(1,sem_id_train);

  //Creation de la semaphore des destinations
  sem_id_dest = initsem_destination(SKEY);

  //Creation file de message par quai
  msgid_1 = msgget(SKEY, 0750 | IPC_CREAT | IPC_EXCL);
  msg_rep_id_1 = msgget(SKEY, 0750 | IPC_CREAT | IPC_EXCL);

  msgid_2 = msgget(SKEY, 0750 | IPC_CREAT | IPC_EXCL);
  msg_rep_id_2 = msgget(SKEY, 0750 | IPC_CREAT | IPC_EXCL);


  //Créations des portiques
  pid_portique1=fork();
  if(pid_portique1==0){
    portique(1);
  }
  else{
    pid_portique2=fork();
    if(pid_portique2==0){
      portique(2);
    }
    else{
      //Creationdes véhicules
      gestionnaire_creation_vehicule();
    }
  }

  waitpid(pid_portique1,&status,0);

  return 0;
}
