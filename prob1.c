/*
	Author(s): Navarre Brown, Ne'kko Montoya, Jason Miller
	Title: prob1.c
	Use: Solves the baboons problem. 
		1) If a baboon begins to cross, it is  guaranteed to get to the other side without running into a baboon going the other way.
		2) There are never more than three baboons on the rope.
		3) A continuing stream of baboons crossing in one direction should not bar baboons going the other way indefinitely (no starvation)
	Date: 11-2022
*/
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

//declare semaphores
sem_t rope;//used to make sure baboons only go one direction at a time
sem_t mutexE;//mutex semaphore for the east side of the canyon
sem_t mutexW;//mutex semaphore for the west side of the canyon
sem_t dp;//deadlock prevention semaphore
sem_t maxOnRope;//semaphore for making sure no more than 3 baboons enter the rope

//declare global variables
int goingEast = 0;
int goingWest = 0;
int timeToCross;    

//
void* east_side(void*arg)            {
    int id = *(int*)arg;//variable for baboon id number
    int remainingSpots = 0;//variable for number of remaining spots on the rope
    
    //critical section
    sem_wait(&dp);
    sem_wait(&mutexE);
    goingEast++;
    if (goingEast == 1){//first instance of that direction
        sem_wait(&rope);//if another direction is on rope, wait until all baboons off rope
        printf("Baboon %d: waiting\n", id);
    }//end if
    sem_post(&mutexE);
    sem_post(&dp);
    
    //section for tracking number of baboons on rope and printing to terminal
    sem_wait(&maxOnRope);//decriment the maxOnRope value by one
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Cross rope request granted (Current crossing: left to right, Number of baboons on rope: %d)\n", id, 3-remainingSpots);//
    sleep(timeToCross);//sleep for amount of time it takes to cross the rope
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Exit rope (Current crossing: left to right, Number of baboons on rope: %d)\n", id, 2-remainingSpots);
    sem_post(&maxOnRope);//incriment the maxOnRope value by one
    
    //critical section
    sem_wait(&mutexE);
    goingEast--;
    if (goingEast == 0){
        sem_post(&rope);//all baboons of this direction off rope, allow for other direction to enter rope
    }//end if
    sem_post(&mutexE);
}//end east_side

//thread handling west to east travel
void* west_side(void*arg){
    int id = *(int*)arg;//variable for baboon id number
    int remainingSpots = 0;//variable for number of remaining spots on the rope
    
    //critical section
    sem_wait(&dp);
    sem_wait(&mutexW);
    goingWest++;
    if (goingWest == 1){//first instance of that direction
        sem_wait(&rope);//if another direction is on rope, wait until all baboons off rope
        printf("Baboon %d: waiting\n", id);
    }//end if
    sem_post(&mutexW);
    sem_post(&dp);
    
    //section for tracking number of baboons on rope and printing to terminal
    sem_wait(&maxOnRope);//decriment the maxOnRope value by one
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Cross rope request granted (Current crossing: right to left, Number of baboons on rope: %d)\n", id, 3-remainingSpots);
    sleep(timeToCross);//sleep for amount of time it takes to cross the rope
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Exit rope (Current crossing: right to left, Number of baboons on rope: %d)\n", id, 2-remainingSpots);
    sem_post(&maxOnRope);//incriment the maxOnRope value by one
    
    //critical section
    sem_wait(&mutexW);
    goingWest--;
    if (goingWest == 0){
        sem_post(&rope);//all baboons of this direction off rope, allow for other direction to enter rope
    }//end if
    sem_post(&mutexW);
}//end west_side

/*main function*/
int main(int argc, char *argv[]){ 
	//initialize vairbales
    char c;											//temp char variable to hold the characters read from the file
    int numBaboons=0;								//total number of baboons
    char temp[100];									//temp char array for input from file
    int baboonsE=0;									//number of baboons on the east side of the canyon
    int baboonsW=0;									//number of baboons on the west side of the canyon
	int numBaboonsE=0;								//tracking the number of threads made for east side of canyon
	int numBaboonsW=0; 								//tracking the number of threads made for west side of canyon
	int idE[baboonsE];								//tracking id for each baboon om east side of canyon
	int idW[baboonsW];								//tracking id for each baboon om west side of canyon
	pthread_t eastSide[baboonsE],westSide[baboonsW];//threads for baboons on the east side and west side if the canyon
	
	
    sem_init(&rope,0,1);                        	//ensure mutual exclusion on rope ownership
    sem_init(&mutexE,0,1);                  		//east side on travel
    sem_init(&mutexW,0,1);                  		//west side on travel
    sem_init(&dp,0,1);         						//used to prevent deadlocks while using semaphores
    sem_init(&maxOnRope,0,3);                     	//ensure only 3 baboons are allowed on the rope

    //ensure all input arguements are entered
    if ( argc != 3 ){
        printf("Must have 2 arguments: \n <input filename> <time to cross>\n");
    }else{
        timeToCross = atoi(argv[2]);
        FILE *file;
        if (file = fopen(argv[1], "r") ){
            while((c=getc(file))!=EOF){
                if(c == 'L'){
                    temp[numBaboons] = c;
                    numBaboons++;
                    baboonsE++;
                }else if(c == 'R'){
                	temp[numBaboons] = c;
                    numBaboons++;
                    baboonsW++;
            	}//end if else
        	}//end while
        }else{
            printf("Unable to read data from the input file.");
            return 0;
        }//end if else
        
        //print the input to the terminal
        printf("The input is\n");
        for(int i = 0; i < numBaboons; i++){
            printf("%c ",temp[i]);
        }//end for
        printf("\n");
        
		for(int i = 0; i < numBaboons; i++){
		    sleep(1);
		    if(temp[i]=='L'){
				idE[numBaboonsE]=i;
				printf("Baboon %d wants to cross left to right\n",i);
				pthread_create(&eastSide[numBaboonsE],NULL, (void *) &east_side,(void *) &idE[numBaboonsE] );
				numBaboonsE++;
		      
		    }else if(temp[i]=='R'){
				idW[numBaboonsW]=i;
				printf("Baboon %d wants to cross right to left\n",i);
				pthread_create(&westSide[numBaboonsW],NULL, (void *) &west_side,(void *) &idW[numBaboonsW] );
				numBaboonsW++;
		    }//end if else if
		 }//end for
        
        for(int i = 0; i < baboonsW; i++){
		    pthread_join(westSide[i],NULL); 
		}//end for
		
		for(int i = 0; i < baboonsE; i++){
		    pthread_join(eastSide[i],NULL); 
		}//end for

        //destroy all semaphores
        sem_destroy (&rope); 
        sem_destroy (&mutexE);
        sem_destroy (&mutexW);
        sem_destroy (&dp);
        sem_destroy (&maxOnRope);
        
        return 0;
    }//end if else
}//end main



