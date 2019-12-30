
//
// Created by kristen
//
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
//#include <sys/time.h>

static int glob = 0;
static sem_t rw_mutex;
static sem_t mutex;
int read_count=0;
//int p=30300; //w=10*30=300, r=500*60=30000
double time_rmin=100000;
double time_rmax=-1;
double time_rtotal=0;
double time_rcnt=0;;

double time_wmin=100000;
double time_wmax=-1;
double time_wtotal=0;
double time_wcnt=0;
//long t;


static void *FuncW(void *arg) {
   
     clock_t tt;
     double ttime;

	 int loops = *((int *) arg);
   
     for (int i=0; i<loops;i++) {
         
         
         tt =clock();//get start time
         
         sem_wait(&rw_mutex);
       
         tt=clock()-tt;//get end time
         
         ttime= ((double)tt/CLOCKS_PER_SEC); //calculate difference
         
         
         time_wtotal +=ttime;
		 time_wcnt++;
		 
         if(time_wmin>ttime) time_wmin=ttime;
         if(time_wmax<ttime) time_wmax=ttime;
           
         
         //write us performed
        
         glob+=10;
          
         sem_post(&rw_mutex);
           
         
         usleep((rand()%100)*1000);
    }

}

static void *FuncR(void *arg) {
	
    clock_t tt;
    double ttime;

    int loops = *((int *) arg);
   
	for (int i=0; i<loops;i++){
      
         tt =clock();//get start time
        
         sem_wait(&mutex);		 
         
        
         read_count++;
        
         if(read_count==1) sem_wait(&rw_mutex);
        
         sem_post(&mutex);
        
         tt=clock()-tt;//get end time
      
         ttime= ((double)tt/CLOCKS_PER_SEC);
         time_rtotal +=ttime;
         time_rcnt++;
         if(time_rmin>ttime) time_rmin=ttime;
         if(time_rmax<ttime) time_rmax=ttime;

         //read is performed
         printf("read num: %d\n", glob);
        
         sem_wait(&mutex);		 
         
       
         
         read_count--;
         if(read_count==0)
                sem_post(&rw_mutex);
        
         sem_post(&mutex);
        
         usleep((rand()%100)*1000);
        }
}


int main(int argc, char *argv[]) {
    if(argc<3){exit(0);}//exit(); if there is less than 2 parameter
    int r_loop=atoi(argv[2]);
    int w_loop=atoi(argv[1]);
    pthread_t t_read[500];
    pthread_t t_write[10];
    int s;

    time_t t;//current time
    /* Intializes random number generator */
    srand((unsigned) time(&t));

    //initialize semaphore re_mutex and mutex
    if (sem_init(&rw_mutex, 0, 1) == -1) {//* 0 (threads not processes)
          printf("Error, init semaphore\n");
          exit(1);
      }
    if (sem_init(&mutex, 0, 1) == -1) {//* 0 (threads not processes)
        printf("Error, init semaphore\n");
        exit(1);
    }

    //create 500 threads for read
    for (int i=0; i<500;i++){
        s = pthread_create(&t_read[i], NULL, FuncR, &r_loop);
          if (s != 0) {
              printf("Error, creating threads1\n");
              exit(1);
          }
    }


    for (int i=0; i<10;i++){
        s = pthread_create(&t_write[i], NULL, FuncW, &w_loop);
          if (s != 0) {
              printf("Error, creating threads2\n");
              exit(1);
          }
    }

  
   for (int i=0; i<500;i++){
   s = pthread_join( t_read[i], NULL);
   if (s != 0) {
    printf("Error, creating threads3\n");
    exit(1);
  }
}
   for (int i=0; i<10;i++){
   s = pthread_join( t_write[i], NULL);
   if (s != 0) {
    printf("Error, creating threads4\n");
    exit(1);
  }
}
    printf("in second\n");
    printf("write:    \n");
    printf("avarg:%f\t\t",time_wtotal/time_wcnt);
    printf("min:%f\t\t",time_wmin);
    printf("max:%f\t\t\n",time_wmax);
    printf("read:     \n");
    printf("avarg:%f\t\t",time_rtotal/time_rcnt);
    printf("min:%f\t\t",time_rmin);
    printf("max:%f\t\t\n",time_rmax);

   /* while(p>0){
        sleep(1);
    }*/

}


