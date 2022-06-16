#include <semaphore.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

//definition of semaphors
sem_t *sem_oxygen;
sem_t *sem_hydrogen;
sem_t *molecule;
sem_t *H_added;
sem_t *H_created;
sem_t *molecule_created;

FILE *fp;

//definition of shared memory varibles line and mol_num
int *line;
int *mol_num;


int oxygen(int id,int NH,int TI,int TB)
{
  //genereate random values for going to que and creating molecule (times of doing the activit)
  int random_que = rand() % TI;
  int random_create = rand() % TB;

  fprintf(fp,"%d: O %d: started\n",(*line)++,id);
  usleep(random_que);
  fprintf(fp,"%d: O %d: going to queue\n",(*line)++,id);
  sem_wait(sem_oxygen); //lock oxygen so only one oxygen can be processed in this section
  sem_wait(molecule); //when we free oxygen we still have to wait to free molecule we just check the value
  sem_post(molecule);
  if((*mol_num-1)!=NH/2) //if we have have enaugh Hydrgon we go on 
  {
    sem_wait(H_added); //waiting for H 
    sem_wait(H_added);
    sem_wait(molecule); // after 2xH_passed molecule lock
    fprintf(fp,"%d: O %d: creating molecule %d\n",(*line)++,id,(*mol_num));
    usleep(random_create);
    sem_post(molecule_created); //signal to unlocking semaphor for H to create molecules
    sem_wait(H_created); //waiting for 2x H to finish creating 
    sem_wait(H_created);
    fprintf(fp,"%d: O %d: molecule %d created\n",(*line)++,id,(*mol_num));
    sem_wait(molecule_created); //lock semaphor for H to create molecule
    sem_post(sem_oxygen);
    (*mol_num)++;
    sem_post(molecule);

    return 0;
  }
  else //if we dont have enaught hydrogen
  {
    sem_post(sem_oxygen);
    fprintf(fp,"%d: O %d: not enough H\n",(*line)++,id);
    return 1;
  }
}

int hydrogen(int id,int oxygen,int NH,int TI)
{

  int random_que = rand() % TI; //generatte random value for going to que

  fprintf(fp,"%d: H %d: started\n",(*line)++,id);
  usleep(random_que);
  fprintf(fp,"%d: H %d: going to queue\n",(*line)++,id);
  sem_wait(sem_hydrogen);//lock hydrogen 
  sem_wait(molecule);//check if the que is open
  sem_post(molecule);
  if((*mol_num)-1<oxygen && (*mol_num)-1<NH/2) //if we have enaugh hydrogen and oxygen go on
  {
    fprintf(fp,"%d: H %d: creating molecule %d\n",(*line)++,id,*mol_num);
    sem_post(H_added);
    sem_wait(molecule_created); // wait for signal from oxygen 
    sem_post(molecule_created);
    fprintf(fp,"%d: H %d: molecule %d created\n",(*line)++,id,*mol_num);
    sem_post(sem_hydrogen);
    sem_post(H_created);
  }
  else // if we dont have enaught O or H 
  {
    fprintf(fp,"%d: H %d: not enough O or H\n",(*line)++,id);
    sem_post(sem_hydrogen);
    return 1;
  }

  return 0;
}

void create_semaphors()
{
  int id ;

  id = shmget ( IPC_PRIVATE , 10 , 0666) ;
  sem_oxygen = ( sem_t *) shmat ( id , ( void *) 0 , 0) ;
  sem_init(sem_oxygen,1,1);

  id = shmget ( IPC_PRIVATE , 10 , 0666) ;
  sem_hydrogen = ( sem_t *) shmat ( id , ( void *) 0 , 0) ;
  sem_init(sem_hydrogen,1,2);

  id = shmget ( IPC_PRIVATE , 10 , 0666) ;
  molecule = ( sem_t *) shmat ( id , ( void *) 0 , 0) ;
  sem_init(molecule,1,1);

  id = shmget ( IPC_PRIVATE , 10 , 0666) ;
  molecule_created = ( sem_t *) shmat ( id , ( void *) 0 , 0) ;
  sem_init(molecule_created,1,1);

  id = shmget ( IPC_PRIVATE , 10 , 0666) ;
  H_created = ( sem_t *) shmat ( id , ( void *) 0 , 0) ;
  sem_init(H_created,1,2);

  id = shmget ( IPC_PRIVATE , 10 , 0666) ;
  H_added = ( sem_t *) shmat ( id , ( void *) 0 , 0) ;
  sem_init(H_added,1,2);

}
int main(int argc,char **argv) {
  //testing program parameters
  int NO;
 if(argc!=5)
  {
    fprintf(stderr,"WRONG NUMBER OF PARAMETERS\n");
    return 1;
  }

  NO=atoi(argv[1]);
  if(NO==0)
  {
    fprintf(stderr,"WRONG FIRST ARGUMENT\n");
    return 1;
  }
  
  int NH;
  NH=atoi(argv[2]);
  if(NH==0){
    fprintf(stderr,"WRONG SECOND ARGUMENT\n");
    return 1;
  }

  int TI; //maximum time to wait for que
  TI=atoi(argv[3]);
  if(TI==0)
  {
    fprintf(stderr,"WRONG THIRD ARGUMENT\n");
    return 1;
  }

  int TB; //maximo time to create molecule
  TB=atoi(argv[4]);
  if(TB==0)
  {
    fprintf(stderr,"WRONG FOURTH ARGUMENT\n");
    return 1;
  }



  //open file
  fp = fopen("proj2.out","w");
  setbuf(fp,NULL);

  //define shared memory variables
  int line_fp;
  int mol_fp;
  line_fp = shmget ( IPC_PRIVATE , 10 , 0666) ;
  line = ( int *) shmat ( line_fp , ( void *) 0 , 0) ;
  *line=1;
  mol_fp = shmget ( IPC_PRIVATE , 10 , 0666) ;
  mol_num = ( int *) shmat ( mol_fp , ( void *) 0 , 0) ;
  *mol_num=1;

  create_semaphors();

  setbuf(stdout,NULL);
  //pre set some semafors
  sem_wait(H_added);
  sem_wait(H_added);
  sem_wait(H_created);
  sem_wait(H_created);
  sem_wait(molecule_created);


  //create oxygen process NO number of oxygens
  for(int id = 1; id <= NO; ++id) {
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr,"error fork");
      return 1;
    }
    if (pid == 0) {
        return oxygen(id,NH,TI,TB);
    }
  }

  //create hydrogen proces NH number of hydrogens
  for(int id = 1; id <= NH; ++id) {
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr,"error fork");
      return 1;
    }
    if (pid == 0) {
      return hydrogen(id,NO,NH,TI);
    }
  }
  return 0;
}