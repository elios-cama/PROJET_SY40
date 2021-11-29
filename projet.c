#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

#define CAPACITE_ENTREPOT 20
#define CAPACITE_QUAI 1
void traitant_vide(int signum) {}


int creer_entrepot(int tube_portique,  int tube_entrepot)
{
  int pid = fork();
  if(pid != 0)
    return pid;

  int stock = 0;
  printf("Stock de l'entrepot: %d\n", stock);

  while(1)
  {
    int valeur;
    read(tube_entrepot, &valeur, sizeof(int));

    stock += valeur;

    printf("Stock de l'entrepot: %d\n", stock);
    

}}

int creer_portique(int tube_portique, int tube_entrepot){
    int pid = fork();
    if(pid != 0)
        return pid;

  printf("portique créé\n");

  sigset_t new_mask, old_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

  signal(SIGUSR1, traitant_vide);
  while(1)
  {
    int vehicule[2];
    read(tube_portique, &vehicule, 2 * sizeof(int));
    printf("Déchargement des containers du vehicule %d en cours\n", vehicule[0]);
    write(tube_entrepot, vehicule + 1, sizeof(int));
    sigsuspend(&old_mask);
    printf("Déchargement des containers du vehicule %d terminé\n", vehicule[0]);
    kill(vehicule[0], SIGUSR2);
  }
}



int creer_quai(int tube_dechargement, int tube_portique)
{
  int pid = fork();
  if(pid != 0)
    return pid;

  printf("Quai de déchargement créé\n");

  sigset_t new_mask, old_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

  signal(SIGUSR1, traitant_vide);

  while(1)
  {
    int vehicule[2];
    read(tube_dechargement, &vehicule, 2 * sizeof(int));
    printf("Déchargement du vehicule %d en cours\n", vehicule[0]);
    write(tube_portique, vehicule + 1, sizeof(int));
    sigsuspend(&old_mask);
    printf("Déchargement du vehicule %d terminé\n", vehicule[0]);
    kill(vehicule[0], SIGUSR2);
  }
}



void creer_vehicule(int tube_dechargement)
{
  int pid = fork();
  if(pid != 0)
    return;

  sigset_t new_mask, old_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGUSR2);
  sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

  signal(SIGUSR2, traitant_vide);

  int container = 2;
  int requete[2] = { getpid(), container };

 
    printf("vehicule %d s'amarre au quai de déchargement et transporte %d containers\n", getpid(), container);
    write(tube_dechargement, requete, 2 * sizeof(int));
    sigsuspend(&old_mask);
    printf("vehicule %d s'en va après avoir déchargé %d container\n", getpid(), container);
  

  exit(0);
}


int nb_vehicule = 1;

void traitant_sigchld(int signum, siginfo_t *info, void *foo)
{
  waitpid(info->si_pid, NULL, 0);
  printf("vehicule %d est parti\n", info->si_pid);
  nb_vehicule--;
}



int main(int argc, char *argv[])
{
  if(argc != 2)
  {
    fprintf(stderr, "Usage: %s <nb_ships>\n", argv[0]);
    return 1;
  }

  int n = nb_vehicule = atoi(argv[1]);

  srand(time(NULL));

  int tube_dechargement[2], tube_entrepot[2];
  pipe(tube_dechargement);
  pipe(tube_entrepot);


  int quai_dechargement = creer_quai(tube_dechargement[0], tube_entrepot[1]);

  int entrepot = creer_entrepot( quai_dechargement, tube_entrepot[0]);

  int nb_bateau_dechargement = 0;
  

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = traitant_sigchld;
  sigaction(SIGCHLD, &sa, NULL);

  int i;
  for(i = 0; i < n; i++)
  {
    sleep(2);
    creer_vehicule( tube_dechargement[1]);
  }

  while(nb_vehicule > 0);

  
  kill(quai_dechargement, SIGTERM);
  kill(entrepot,          SIGTERM);

  return 0;
}
