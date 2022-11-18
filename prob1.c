/*
	Author(s): Jason Miller, Navarre Brown, Ne'kko Montoya 
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
sem_t mutexL;//mutex semaphore for the left side of the canyon
sem_t mutexR;//mutex semaphore for the right side of the canyon
sem_t dp;//deadlock prevention semaphore
sem_t maxOnRope;//semaphore for making sure no more than 3 baboons enter the rope

//declare global variables
int goingRight = 0;
int goingLeft = 0;
int timeToCross;    

//
void* leftSide(void*arg)            {
    int id = *(int*)arg;//variable for baboon id number
    int remainingSpots = 0;//variable for number of remaining spots on the rope
    
    //critical section
    sem_wait(&dp);
    sem_wait(&mutexL);
    goingRight++;
    if (goingRight == 1){//first instance of that direction
        sem_wait(&rope);//if another direction is on rope, wait until all baboons off rope
        printf("Baboon %d: waiting\n", id);
    }//end if
    sem_post(&mutexL);
    sem_post(&dp);
    
    //section for tracking number of baboons on rope and printing to terminal
    sem_wait(&maxOnRope);//decriment the maxOnRope value by one
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Allowed to start crossing left to right (Number of baboons on rope: %d)\n", id, 3-remainingSpots);//
    sleep(timeToCross);//sleep for amount of time it takes to cross the rope
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Finished crossing left to right rope (Number of baboons on rope: %d)\n", id, 2-remainingSpots);
    sem_post(&maxOnRope);//incriment the maxOnRope value by one
    
    //critical section
    sem_wait(&mutexL);
    goingRight--;
    if (goingRight == 0){
        sem_post(&rope);//all baboons of this direction off rope, allow for other direction to enter rope
    }//end if
    sem_post(&mutexL);
}//end leftSide

//thread handling west to east travel
void* rightSide(void*arg){
    int id = *(int*)arg;//variable for baboon id number
    int remainingSpots = 0;//variable for number of remaining spots on the rope
    
    //critical section
    sem_wait(&dp);
    sem_wait(&mutexR);
    goingLeft++;
    if (goingLeft == 1){//first instance of that direction
        sem_wait(&rope);//if another direction is on rope, wait until all baboons off rope
        printf("Baboon %d: waiting\n", id);
    }//end if
    sem_post(&mutexR);
    sem_post(&dp);
    
    //section for tracking number of baboons on rope and printing to terminal
    sem_wait(&maxOnRope);//decriment the maxOnRope value by one
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Allowed to start crossing right to left (Number of baboons on rope: %d)\n", id, 3-remainingSpots);
    sleep(timeToCross);//sleep for amount of time it takes to cross the rope
    sem_getvalue(&maxOnRope, &remainingSpots);//save current value of maxOnRope value into remainingSpots variable
    printf("Baboon %d: Finished crossing right to left (Number of baboons on rope: %d)\n", id, 2-remainingSpots);
    sem_post(&maxOnRope);//incriment the maxOnRope value by one
    
    //critical section
    sem_wait(&mutexR);
    goingLeft--;
    if (goingLeft == 0){
        sem_post(&rope);//all baboons of this direction off rope, allow for other direction to enter rope
    }//end if
    sem_post(&mutexR);
}//end rightSide

/*main function*/
int main(int argc, char *argv[]){ 
	//initialize vairbales
    char c;											//temp char variable to hold the characters read from the file
    int numBaboons=0;								//total number of baboons
    char temp[100];									//temp char array for input from file
    int baboonsL=0;									//number of baboons on the left side of the canyon
    int baboonsR=0;									//number of baboons on the right side of the canyon
	int numbaboonsL=0;								//tracking the number of threads made for left side of canyon
	int numbaboonsR=0; 								//tracking the number of threads made for right side of canyon
	int leftID[baboonsL];								//tracking id for each baboon om east side of canyon
	int rightID[baboonsR];								//tracking id for each baboon om west side of canyon
	pthread_t left[baboonsL],right[baboonsR];//threads for baboons on the left side and right side if the canyon
	
	
    sem_init(&rope,0,1);                        	//ensure mutual exclusion on rope ownership
    sem_init(&mutexL,0,1);                  		//east side on travel
    sem_init(&mutexR,0,1);                  		//west side on travel
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
                    baboonsL++;
                }else if(c == 'R'){
                	temp[numBaboons] = c;
                    numBaboons++;
                    baboonsR++;
            	}//end if else
        	}//end while
        }else{
            printf("Unable to read data from the input file.\n Please make sure arguments in order <input filename> <time to cross>\n");
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
				leftID[numbaboonsL]=i;
				printf("Baboon %d wants to cross left to right\n",i);
				pthread_create(&left[numbaboonsL],NULL, (void *) &leftSide,(void *) &leftID[numbaboonsL] );
				numbaboonsL++;
		      
		    }else if(temp[i]=='R'){
				rightID[numbaboonsR]=i;
				printf("Baboon %d wants to cross right to left\n",i);
				pthread_create(&right[numbaboonsR],NULL, (void *) &rightSide,(void *) &rightID[numbaboonsR] );
				numbaboonsR++;
		    }//end if else if
		 }//end for
        
        for(int i = 0; i < baboonsR; i++){
		    pthread_join(right[i],NULL); 
		}//end for
		
		for(int i = 0; i < baboonsL; i++){
		    pthread_join(left[i],NULL); 
		}//end for

        //destroy all semaphores
        sem_destroy (&rope); 
        sem_destroy (&mutexL);
        sem_destroy (&mutexR);
        sem_destroy (&dp);
        sem_destroy (&maxOnRope);
        
        return 0;
    }//end if else
}//end main



