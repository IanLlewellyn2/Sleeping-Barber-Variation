/*special flag for compiler/linker: -pthread
Ian Llewellyn - Project 3 CS4348

Use : csmc #students #tutors #chairs #help

Assumptions
-Students will leave/ go program after getting helped once and wait between 1 and MAX_TIME_BETWEEN_VISITS seconds to visit again
-When called to tutor, tutors will spend between 1 and MAX_TUTOR_HELP_TIME seconds tutoring the student before going back to sleep (blocked by semaphore)
-After they start getting helped by a tutor, a student will spend between MAX_TUTOR_HELP_TIME + 1 and 2 * MAX_TUTOR_HELP_TIME before they leave
-Students will give up their seat when they start getting tutored
*/

#include<semaphore.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/ipc.h>
#include<pthread.h>
#include<time.h>

//max time between students entering the csmc
#define MAX_TIME_BETWEEN_VISITS 10
//max number of seconds a tutor will help a student
#define MAX_TUTOR_HELP_TIME 5

int timesToHelp = 0;
//students * times each student needs help
//so tutor and coord threads know when they have helped all students
int totalStudentHelp;

sem_t mutex, tutorNeeded, studentNeeded;
int numStudentsWaiting = 0;
int numTutorsWaiting = 0;
int numChairsFree = 0;

void studentMethod(void *arg);
void tutorMethod(void *arg);
void coordMethod(void *arg);
int studentGoingToWait(int, int);
void tutorLoop(int);

void coordMethod(void *arg)
{
	int i;
	//while there are still students that need help
	while(totalStudentHelp > 0)
	{
		//lock mutex
		sem_wait(&mutex);
		
		//check if there is a waiting tutor and a waiting student
		//if there are, signal for a tutor to help that student
		//signal a waiting student as well
		if(numTutorsWaiting > 0 && numStudentsWaiting > 0)
		{
			sem_post(&tutorNeeded);
			sem_post(&studentNeeded);
		}
		//release mutex
		sem_post(&mutex);
		
		//sleep for 1 second then check again
		//my instinct is that this isnt necessary but it breaks without it
		sleep(1);
	}
	
//	printf("End of coord method\n");
}

void studentMethod(void* i)
{
	int sleepTime;
	int waiting = 0; //is the student already waiting in the waiting room
	//getting the number of this student, free memory after
	int stuNum = *((int *) i);
    free(i);
	
	//this student's number of times they still need help, when it hits 0 the student will leave
	int timesToBeHelped = timesToHelp;
	
	//student seeks help until they have been helped as many times a specified in terminal arg
	while(timesToBeHelped > 0)
	{
		//student is not waiting in CSMC
		//sleep for some random amount of time so students walk in at different times - seeded in main	
		//may occur multiple times if no chairs are open when the student arrives
		waiting = studentGoingToWait(waiting, stuNum);
		
		//student now has a chair, waits to be called by coordinator
		if(waiting == 1)
		{
			//student getting help from a tutor
			sem_wait(&studentNeeded);
			timesToBeHelped--;
			
			//got help, gave up seat - tutor will mark that the seat is free and 1 less student is waiting
			waiting = 0;
					
			//student will sleep during and a short random time after getting help from the tutor
			//After they start getting helped by a tutor, a student will spend between MAX_TUTOR_HELP_TIME + 1 and 2 * MAX_TUTOR_HELP_TIME before they leave/go back to programming
			sleepTime = (rand() % MAX_TUTOR_HELP_TIME) + 1;
			sleep(MAX_TUTOR_HELP_TIME + sleepTime);
			
			//student goes to program for a set amount of time			
		}
	}	
	//student is done at CSMC, does not need to return
}
int studentGoingToWait(int waiting, int stu)
{
	//sleeping between requests for tutoring
	//can simulate: a) student has not yet been helped by the csmc at all
	//b) student is programming between requests for tutoring
	int sleepTime = (rand() % MAX_TIME_BETWEEN_VISITS) + 1;
	sleep(sleepTime);

	//student arrives, check if there are any empty chairs
	sem_wait(&mutex); //lock mutex
	if(numChairsFree > 0)
	{
		//take a chair
		numChairsFree--;
		//student is now waiting
		numStudentsWaiting++;
		waiting = 1;
		printf("Student %d takes a seat. Waiting students = %d\n", stu, numStudentsWaiting);
	}
	else
		printf("Student %d found no empty chair will try again later \n", stu);
	//release mutex
	sem_post(&mutex);
	return waiting;
}

void tutorMethod(void *i)
{
	int *j;
	//getting the number of this tutor
	int tutorNum = *((int *) i);
    free(i);
	
	//tutor is here, let coordinator know
	sem_wait(&mutex);
	numTutorsWaiting++;
	sem_post(&mutex);
	
	//while students still need help
	while(totalStudentHelp > 0)
	{
		tutorLoop(tutorNum);
		
	}
}

void tutorLoop(int tutorID)
{
	//wait until a tutor is needed
	sem_wait(&tutorNeeded);
			
	sem_wait(&mutex);
				
	//tutor needed, change value so coordinator will know
	numTutorsWaiting--;
	numStudentsWaiting--;
	numChairsFree++;
		
	//help student, sleep to simulate doing so
	int sleepTime = (rand() % MAX_TUTOR_HELP_TIME) + 1;		
	totalStudentHelp--;
	printf("Tutor %d helping student for %d seconds. Waiting students = %d\n", tutorID, sleepTime, numStudentsWaiting);
	sem_post(&mutex);
	sleep(sleepTime);
	
	//student has been helped, let coordinator know you are waiting
	sem_wait(&mutex);
	numTutorsWaiting++;
	sem_post(&mutex);
}

int main(int argc, char * argv[])
{
	srand(time(NULL)); //seed RNG
	
	//initial values given through terminal
	if(argc != 5)
		perror("You must enter four arguments!");
	
	int numStudents = atoi(argv[1]);
	int numTutors = atoi( argv[2]);
	int numChairs = atoi( argv[3]);
	timesToHelp = atoi(argv[4]);

	
	
	//number of total times tutors will have to help
	totalStudentHelp = numStudents * timesToHelp;
	
	//all chairs start out free
	numChairsFree = numChairs;
	
	//for loop iterator
	int i = 0;
	
	//pthread ids
	pthread_t coord;
	pthread_t students[numStudents];
	pthread_t tutors[numTutors];	
	
	//status for creating pthreads
	int status;
	
	
	//initialize semaphore
	sem_init(&mutex, 0, 1);
	sem_init(&tutorNeeded, 0, 0);
	sem_init(&studentNeeded, 0, 0);
	
	
	//create coordinator thread
	status = pthread_create(&coord, NULL, (void *)coordMethod, NULL);
	if(status != 0)
			perror("Error creating coord thread");
		
	
	//initialize student threads	
	for(i = 0; i < numStudents; i++)
	{
		//allocate student number onto heap so thread can access later
		int *j = malloc(sizeof(*j));
		*j = i;
		//create thread
		status = pthread_create(&students[i], NULL, (void *)studentMethod, j);
		if(status != 0)
			perror("Error creating student thread");
	}
	
	//initialize tutor threads
	for(i = 0; i < numTutors; i++)
	{
		//allocate tutor number onto heap so thread can access later
		int *j = malloc(sizeof(*j));
		*j = i;
		status = pthread_create(&tutors[i], NULL, (void *)tutorMethod, j);
		if(status != 0)
			perror("Error creating tutor thread");
	}
	
	
	
	//join threads when finished
	for(i = 0; i < numStudents; i++)
	{
		pthread_join(students[i], NULL);
	}	
	pthread_join(coord, NULL);
	for(i = 0; i < numTutors; i++)
	{
		//using pthread_cancel to make sure all tutor threads get closed
		//otherwise tutor threads usually get stuck on tutorsNeeded lock and the program will not end
		pthread_cancel(tutors[i]);
		pthread_join(tutors[i], NULL);
	}
	
	return 0;
}