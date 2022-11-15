/*include header files*/
#define SHMKEY ((key_t) 1497)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
//#include <stdbool.h>

//define structer of the buffer
typedef struct{
	int onRope;
}numOnRope;

//delcare pointer to buffer structure
numOnRope *buffer;

//compile with command 
//gcc -o main *.c -lpthread -lrt

/*semaphores*/
sem_t rope;
sem_t east_mutex;
sem_t west_mutex;
sem_t full;
sem_t counter;

/*global variables*/
int east = 0;
int west = 0;
int travel_time;    

//thread handling the east to west to travel
void* east_side(void*arg)            {
    int baboon = *(int*)arg;
    int on_rope = 0;
    sem_wait(&full);
    sem_wait(&east_mutex);
    east++;
    if (east == 1){
        sem_wait(&rope);
        printf("Baboon %d: waiting\n", baboon);
    }//end if
    sem_post(&east_mutex);
    sem_post(&full);
    sem_wait(&counter);
    //sem_getvalue(&counter, &on_rope);
    printf("Baboon %d: Cross rope request granted (Current crossing: left to right, Number of baboons on rope: %d)\n", baboon, 3-on_rope);
    sleep(travel_time);
    sem_getvalue(&counter, &on_rope);
    printf("Baboon %d: Exit rope (Current crossing: left to right, Number of baboons on rope: %d)\n", baboon, 2-on_rope);
    sem_post(&counter);
    sem_wait(&east_mutex);
    east--;
    if (east == 0){
        sem_post(&rope);
    }//end if
    sem_post(&east_mutex);
}//end east_side

//thread handling west to east travel
void* west_side(void*arg){
    int baboon = *(int*)arg;
    int on_rope;
    sem_wait(&full);
    sem_wait(&west_mutex);
    west++;
    if (west == 1){
        sem_wait(&rope);
        printf("Baboon %d: waiting\n", baboon);
    }//end if
    sem_post(&west_mutex);
    sem_post(&full);
    sem_wait(&counter);
    sem_getvalue(&counter, &on_rope);
    printf("Baboon %d: Cross rope request granted (Current crossing: right to left, Number of baboons on rope: %d)\n", baboon, 3-on_rope);
    sleep(travel_time);
    sem_getvalue(&counter, &on_rope);
    printf("Baboon %d: Exit rope (Current crossing: right to left, Number of baboons on rope: %d)\n", baboon, 2-on_rope);
    sem_post(&counter);
    sem_wait(&west_mutex);
    west--;
    if (west == 0){
        sem_post(&rope);
    }//end if
    sem_post(&west_mutex);
}//end west_side

/*main function*/
int main(int argc, char *argv[]){ 
	//initialize vairbales
    char c;										//temp char variable to hold the characters read from the file
    int baboonCnt=0;							//total number of baboons
    char temp[100];								//temp char array for input from file
    int eastBab=0;								//number of baboons on the east side of the canyon
    int westBab=0;								//number of baboons on the west side of the canyon
	int eastcnt=0;								//tracking the number of threads made for east side of canyon
	int westcnt=0; 								//tracking the number of threads made for west side of canyon
	int eastid[eastBab];						//tracking id for each baboon om east side of canyon
	int westid[westBab];						//tracking id for each baboon om west side of canyon
	pthread_t eastern[eastBab],western[westBab]; //threads for baboons on the east side and west side if the canyon
	int shmid;
	char *shmadd;
	
	
    sem_init(&rope,0,1);                        //ensure mutual exclusion on rope ownership
    sem_init(&east_mutex,0,1);                  //east side on travel
    sem_init(&west_mutex,0,1);                  //west side on travel
    sem_init(&full,0,1);         //used to prevent deadlocks while using semaphores
    sem_init(&counter,0,3);                     //ensure only 3 baboons are allowed on the rope
    
    //create and connect to a shared memory segment
	if ((shmid = shmget (SHMKEY, sizeof(int), IPC_CREAT | 0666)) < 0){
    	perror ("shmget");
    	exit (1);     
    }//end if
    
 	if ((buffer = (numOnRope *) shmat (shmid, shmadd, 0)) == (numOnRope *) - 1) {
    	perror ("shmat");
    	exit (0);
    }//end if
    
    //set shared value onRope to 0. This will be used to track the number of baboons on the rope
    buffer->onRope = 0;

    //ensure all input arguements are entered
    if ( argc != 3 ){
        printf("Must have 2 arguments: \n <input filename> <time to cross>\n");
    }else{
        travel_time = atoi(argv[2]);
        FILE *file;
        if (file = fopen(argv[1], "r") ){
            while((c=getc(file))!=EOF){
                if(c == 'L'){
                    temp[baboonCnt] = c;
                    baboonCnt++;
                    eastBab++;
                }else if(c == 'R'){
                	temp[baboonCnt] = c;
                    baboonCnt++;
                    westBab++;
            	}//end if else
        	}//end while
        }else{
            printf("Unable to read data from the input file.");
            return 0;
        }//end if else
        
        //print the input to the terminal
        printf("The input is\n");
        for(int i = 0; i < baboonCnt; i++){
            printf("%c ",temp[i]);
        }//end for
        printf("\n");
        
		for(int i = 0; i < baboonCnt; i++){
		    sleep(1);
		    if(temp[i]=='L'){
				eastid[eastcnt]=i;
				printf("Baboon %d wants to cross left to right\n",i);
				pthread_create(&eastern[eastcnt],NULL, (void *) &east_side,(void *) &eastid[eastcnt] );
				eastcnt++;
		      
		    }else if(temp[i]=='R'){
				westid[westcnt]=i;
				printf("Baboon %d wants to cross right to left\n",i);
				pthread_create(&western[westcnt],NULL, (void *) &west_side,(void *) &westid[westcnt] );
				westcnt++;
		    }//end if else if
		 }//end for
        
        for(int i = 0; i < westBab; i++){
		    pthread_join(western[i],NULL); 
		}//end for
		
		for(int i = 0; i < eastBab; i++){
		    pthread_join(eastern[i],NULL); 
		}//end for

        //destroy all semaphores
        sem_destroy (&rope); 
        sem_destroy (&east_mutex);
        sem_destroy (&west_mutex);
        sem_destroy (&full);
        sem_destroy (&counter);
        
        //detatch shared memory
		if (shmdt(buffer) == -1){
		 	perror ("shmdt");
		    exit (-1);
		 }//end if
		 	
		 //remove a shared memory
		 shmctl(shmid, IPC_RMID, NULL);
        return 0;
    }//end if else
}//end main



