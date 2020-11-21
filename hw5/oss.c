/*
 * oss.c November 21, 2020
 * Jared Diehl (jmddnb@umsystem.edu)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "shared.h"
#include "queue.h"

/* Static GLOBAL variable (misc) */
static FILE *fpw = NULL;
static char *exe_name;
static key_t key;
static Queue *queue;
static Time forkclock;
static Data data;
/* -------------------------------------------------- */

/* Static GLOBAL variable (shared memory) */
static int mqueueid = -1;
static Message master_message;
static int shmclock_shmid = -1;
static Time *shmclock_shmptr = NULL;
static int semid = -1;
static struct sembuf sema_operation;
static int pcbt_shmid = -1;
static PCB *pcbt_shmptr = NULL;
/* -------------------------------------------------- */

/* Static GLOBAL variable (fork) */
static int fork_number = 0;
static pid_t pid = -1;
static unsigned char bitmap[PROCESSES_MAX];
/* -------------------------------------------------- */

/* Prototype Function */
void masterInterrupt(int seconds);
void masterHandler(int signum);
void exitHandler(int signum);
void timer(int seconds);
void finalize();
void discardShm(int shmid, void *shmaddr, char *shm_name, char *exe_name, char *process_type);
void cleanUp();
void semaLock(int sem_index);
void semaRelease(int sem_index);
void incShmclock();

void initResource(Data *data);
void displayResource(FILE *fpw, Data data);
void updateResource(Data *data, PCB *pcb);

void initPCBT(PCB *pcbt);
void initPCB(PCB *pcb, int index, pid_t pid, Data data);

void setMatrix(PCB *pcbt, Queue *queue, int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int count);
void calculateNeedMatrix(Data *data, int need[][RESOURCES_MAX], int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int count);
void displayVector(FILE *fpw, int *line_count, char *v_name, char *l_name, int vector[RESOURCES_MAX]);
void displayMatrix(FILE *fpw, int *line_count, char *m_name, Queue *queue, int matrix[][RESOURCES_MAX], int count);
bool bankerAlgorithm(FILE *fpw, int *line_count, bool verbose, Data *data, PCB *pcbt, Queue *queue, int c_index);
/* -------------------------------------------------- */

/* ====================================================================================================
MAIN
==================================================================================================== */
int main(int argc, char *argv[])
{
	/* =====Initialize resources===== */
	exe_name = argv[0];
	srand(getpid());

	//--------------------------------------------------
	/* =====Options===== */
	//Optional Variables
	char log_file[256] = "log.dat";
	bool verbose = false;
	int line_count = 0; //NOTE: file line count is not implemented, but skeleton structure is setup.

	int opt;
	while ((opt = getopt(argc, argv, "hl:v")) != -1)
	{
		switch (opt)
		{
		case 'h':
			printf("NAME:\n");
			printf("	%s - simulate the resource management module with the help of deadlock avoidance strategy.\n", exe_name);
			printf("\nUSAGE:\n");
			printf("	%s [-h] [-l logfile] [-t time].\n", exe_name);
			printf("\nDESCRIPTION:\n");
			printf("	-h          : print the help page and exit.\n");
			printf("	-l filename : the log file used (default log.dat).\n");
			printf("	-v          : turn on verbose mode (default is turn off).\n");
			exit(0);

		case 'l':
			strncpy(log_file, optarg, 255);
			fprintf(stderr, "Your new log file is: %s\n", log_file);
			break;

		case 'v':
			verbose = true;
			break;

		default:
			fprintf(stderr, "%s: please use \"-h\" option for more info.\n", exe_name);
			exit(EXIT_FAILURE);
		}
	}

	//Check for extra arguments
	if (optind < argc)
	{
		fprintf(stderr, "%s ERROR: extra arguments was given! Please use \"-h\" option for more info.\n", exe_name);
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Initialize LOG File===== */
	fpw = fopen(log_file, "w");
	if (fpw == NULL)
	{
		fprintf(stderr, "%s ERROR: unable to write the output file.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Initialize BITMAP===== */
	//Zero out all elements of bit map
	memset(bitmap, '\0', sizeof(bitmap));

	//--------------------------------------------------
	/* =====Initialize message queue===== */
	//Allocate shared memory if doesn't exist, and check if it can create one. Return ID for [message queue] shared memory
	key = ftok("./oss.c", 1);
	mqueueid = msgget(key, IPC_CREAT | 0600);
	if (mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [message queue] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Initialize [shmclock] shared memory===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [shmclock] shared memory
	key = ftok("./oss.c", 2);
	shmclock_shmid = shmget(key, sizeof(Time), IPC_CREAT | 0600);
	if (shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [shmclock] shared memory
	shmclock_shmptr = shmat(shmclock_shmid, NULL, 0);
	if (shmclock_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Initialize shared memory attribute of [shmclock] and forkclock
	shmclock_shmptr->s = 0;
	shmclock_shmptr->ns = 0;
	forkclock.s = 0;
	forkclock.ns = 0;

	//--------------------------------------------------
	/* =====Initialize semaphore===== */
	//Creating 3 semaphores elements
	//Create semaphore if doesn't exist with 666 bits permission. Return error if semaphore already exists
	key = ftok("./oss.c", 3);
	semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
	if (semid == -1)
	{
		fprintf(stderr, "%s ERROR: failed to create a new private semaphore! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Initialize the semaphore(s) in our set to 1
	semctl(semid, 0, SETVAL, 1); //Semaphore #0: for [shmclock] shared memory

	//--------------------------------------------------
	/* =====Initialize process control block table===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [pcbt] shared memory
	key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(PCB) * PROCESSES_MAX;
	pcbt_shmid = shmget(key, process_table_size, IPC_CREAT | 0600);
	if (pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [pcbt] shared memory
	pcbt_shmptr = shmat(pcbt_shmid, NULL, 0);
	if (pcbt_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Init process control block table variable
	initPCBT(pcbt_shmptr);

	//--------------------------------------------------
	/* ===== Queue/Resource ===== */
	//Set up queue
	queue = queue_create();
	initResource(&data);
	displayResource(fpw, data);

	//--------------------------------------------------
	/* =====Signal Handling===== */
	masterInterrupt(TIMEOUT);

	//--------------------------------------------------
	/* =====Multi Processes===== */
	int last_index = -1;
	bool is_bitmap_open = false;
	while (1)
	{
		//int spawn_nano = rand() % 500000000 + 1000000;
		int spawn_nano = 100;
		if (forkclock.ns >= spawn_nano)
		{
			//Reset forkclock
			forkclock.ns = 0;

			//Do bitmap has an open spot?
			is_bitmap_open = false;
			int count_process = 0;
			while (1)
			{
				last_index = (last_index + 1) % PROCESSES_MAX;
				uint32_t bit = bitmap[last_index / 8] & (1 << (last_index % 8));
				if (bit == 0)
				{
					is_bitmap_open = true;
					break;
				}
				else
				{
					is_bitmap_open = false;
				}

				if (count_process >= PROCESSES_MAX - 1)
				{
					//DEBUG: fprintf(stderr, "%s: bitmap is full (size: %d)\n", exe_name, MAX_PROCESS);
					break;
				}
				count_process++;
			}

			//Continue to fork if there are still space in the bitmap
			if (is_bitmap_open == true)
			{
				pid = fork();

				if (pid == -1)
				{
					fprintf(stderr, "%s (Master) ERROR: %s\n", exe_name, strerror(errno));
					finalize();
					cleanUp();
					exit(0);
				}

				if (pid == 0) //Child
				{
					//Simple signal handler: this is for mis-synchronization when timer fire off
					signal(SIGUSR1, exitHandler);

					//Replaces the current running process with a new process (user)
					char exec_index[BUFFER_LENGTH];
					sprintf(exec_index, "%d", last_index);
					int exect_status = execl("./user", "./user", exec_index, NULL);
					if (exect_status == -1)
					{
						fprintf(stderr, "%s (Child) ERROR: execl fail to execute at index [%d]! Exiting...\n", exe_name, last_index);
					}

					finalize();
					cleanUp();
					exit(EXIT_FAILURE);
				}
				else //Parent
				{
					//Increment the total number of fork in execution
					fork_number++;

					//Set the current index to one bit (meaning it is taken)
					bitmap[last_index / 8] |= (1 << (last_index % 8));

					//Initialize user process information to the process control block table
					initPCB(&pcbt_shmptr[last_index], last_index, pid, data);

					//Add the process to highest queue
					queue_push(queue, last_index);

					//Display creation time
					fprintf(stderr, "%s: generating process with PID (%d) [%d] and putting it in queue at time %d.%d\n", exe_name,
							pcbt_shmptr[last_index].spid, pcbt_shmptr[last_index].pid, shmclock_shmptr->s, shmclock_shmptr->ns);
					fprintf(fpw, "%s: generating process with PID (%d) [%d] and putting it in queue at time %d.%d\n", exe_name,
							pcbt_shmptr[last_index].spid, pcbt_shmptr[last_index].pid, shmclock_shmptr->s, shmclock_shmptr->ns);
					fflush(fpw);
				}
			} //END OF: is_bitmap_open if check
		}	  //END OF: forkclock.nanosecond if check

		//- CRITICAL SECTION -//
		incShmclock();

		//--------------------------------------------------
		/* =====Main Driver Procedure===== */
		//Application procedure queues
		QueueNode next;
		Queue *trackingQueue = queue_create();

		int current_iteration = 0;
		next.next = queue->front;
		while (next.next != NULL)
		{
			//- CRITICAL SECTION -//
			incShmclock();

			//Sending a message to a specific child to tell him it is his turn
			int c_index = next.next->index;
			master_message.mtype = pcbt_shmptr[c_index].pid;
			master_message.index = c_index;
			master_message.childPid = pcbt_shmptr[c_index].pid;
			msgsnd(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 0);
			//DEBUG fprintf(fpw, "%s: process with PID (%d) [%d], c:%d\n", exe_name, master_message.index, master_message.childPid, c_index);

			//Waiting for the specific child to respond back
			msgrcv(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 1, 0);

			//- CRITICAL SECTION -//
			incShmclock();

			//If child want to terminate, skips the current iteration of the loop and continues with the next iteration
			if (master_message.flag == 0)
			{
				fprintf(stderr, "%s: process with PID (%d) [%d] has finish running at my time %d.%d\n",
						exe_name, master_message.index, master_message.childPid, shmclock_shmptr->s, shmclock_shmptr->ns);
				fprintf(fpw, "%s: process with PID (%d) [%d] has finish running at my time %d.%d\n",
						exe_name, master_message.index, master_message.childPid, shmclock_shmptr->s, shmclock_shmptr->ns);
				fflush(fpw);

				//Remove the process out of the queue
				QueueNode current;
				current.next = queue->front;
				while (current.next != NULL)
				{
					if (current.next->index != c_index)
					{
						queue_push(trackingQueue, current.next->index);
					}

					//Point the pointer to the next queue element
					current.next = (current.next->next != NULL) ? current.next->next : NULL;
				}

				//Reassigned the current queue
				while (!queue_empty(queue))
				{
					queue_pop(queue);
				}
				while (!queue_empty(trackingQueue))
				{
					int i = trackingQueue->front->index;
					//DEBUG fprintf(stderr, "Tracking Queue i: %d\n", i);
					queue_push(queue, i);
					queue_pop(trackingQueue);
				}

				//Point the pointer to the next queue element
				next.next = queue->front;
				int i;
				for (i = 0; i < current_iteration; i++)
				{
					next.next = (next.next->next != NULL) ? next.next->next : NULL;
				}
				continue;
			}
			//DEBUG fprintf(fpw, "%s: continue | process with PID (%d) [%d], c:%d\n", exe_name, master_message.index, master_message.childPid, c_index);

			//If process is requesting resources, execute the Banker Algorithm
			//If not, simply add the current process to the tracking queue and point the pointer to the next queue element
			if (master_message.isRequest == true)
			{
				fprintf(stderr, "%s: process with PID (%d) [%d] is REQUESTING resources. Invoking banker's algorithm...\n",
						exe_name, master_message.index, master_message.childPid);
				fprintf(fpw, "%s: process with PID (%d) [%d] is REQUESTING resources. Invoking banker's algorithm...\n",
						exe_name, master_message.index, master_message.childPid);
				fflush(fpw);

				//Execute the Banker Algorithm
				bool isSafe = bankerAlgorithm(fpw, &line_count, verbose, &data, pcbt_shmptr, queue, c_index);

				//Send a message to child process whether if it safe to proceed the request OR not
				master_message.mtype = pcbt_shmptr[c_index].pid;
				master_message.isSafe = (isSafe) ? true : false;
				msgsnd(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 0);
			}

			//- CRITICAL SECTION -//
			incShmclock();

			//Is the process releasing resources?
			if (master_message.isRelease)
			{
				fprintf(stderr, "%s: process with PID (%d) [%d] is RELEASING allocated resources.\n",
						exe_name, master_message.index, master_message.childPid);
				fprintf(fpw, "%s: process with PID (%d) [%d] is RELEASING allocated resources.\n",
						exe_name, master_message.index, master_message.childPid);
				fflush(fpw);
			}

			//Increase iterration
			current_iteration++;

			//Point the pointer to the next queue element
			next.next = (next.next->next != NULL) ? next.next->next : NULL;
		} //END OF: next.next
		free(trackingQueue);

		//- CRITICAL SECTION -//
		incShmclock();

		//--------------------------------------------------
		//Check to see if a child exit, wait no bound (return immediately if no child has exit)
		int child_status = 0;
		pid_t child_pid = waitpid(-1, &child_status, WNOHANG);

		//Set the return index bit back to zero (which mean there is a spot open for this specific index in the bitmap)
		if (child_pid > 0)
		{
			int return_index = WEXITSTATUS(child_status);
			bitmap[return_index / 8] &= ~(1 << (return_index % 8));
		}

		//--------------------------------------------------
		//End the infinite loop when reached X forking times. Turn off timer to avoid collision.
		if (fork_number >= PROCESSES_TOTAL)
		{
			timer(0);
			masterHandler(0);
		}
	} //END OF: infinite while loop #1

	return EXIT_SUCCESS;
}

/* ====================================================================================================
* Function    :  masterInterrupt(), masterHandler(), and exitHandler()
* Definition  :  Interrupt master process base on given time or user interrupt via keypress. Send a 
                  terminate signal to all the child process and "user" process. Finally, invoke clean up
                  function.
* Parameter   :  Number of second.
* Return      :  None.
==================================================================================================== */
void masterInterrupt(int seconds)
{
	//Invoke timer for termination
	timer(seconds);

	//Signal Handling for: SIGALRM
	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &masterHandler;
	sa1.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa1, NULL) == -1)
	{
		perror("ERROR");
	}

	//Signal Handling for: SIGINT
	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &masterHandler;
	sa2.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa2, NULL) == -1)
	{
		perror("ERROR");
	}

	//Signal Handling for: SIGUSR1
	signal(SIGUSR1, SIG_IGN);
}
void masterHandler(int signum)
{
	finalize();

	//Print out basic statistic
	fprintf(stderr, "- Master PID: %d\n", getpid());
	fprintf(stderr, "- Number of forking during this execution: %d\n", fork_number);
	fprintf(stderr, "- Final simulation time of this execution: %d.%d\n", shmclock_shmptr->s, shmclock_shmptr->ns);

	cleanUp();

	//Final check for closing log file
	if (fpw != NULL)
	{
		fclose(fpw);
		fpw = NULL;
	}

	exit(EXIT_SUCCESS);
}
void exitHandler(int signum)
{
	printf("%d: Terminated!\n", getpid());
	exit(EXIT_SUCCESS);
}

/* ====================================================================================================
* Function    :  timer()
* Definition  :  Create a timer that decrement in real time. Once the timer end, send out SIGALRM.
* Parameter   :  Number of second.
* Return      :  None.
==================================================================================================== */
void timer(int seconds)
{
	//Timers decrement from it_value to zero, generate a signal, and reset to it_interval.
	//A timer which is set to zero (it_value is zero or the timer expires and it_interval is zero) stops.
	struct itimerval value;
	value.it_value.tv_sec = seconds;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;

	if (setitimer(ITIMER_REAL, &value, NULL) == -1)
	{
		perror("ERROR");
	}
}

/* ====================================================================================================
* Function    :  finalize()
* Definition  :  Send a SIGUSR1 signal to all the child process and "user" process.
* Parameter   :  None.
* Return      :  None.
==================================================================================================== */
void finalize()
{
	fprintf(stderr, "\nLimitation has reached! Invoking termination...\n");
	kill(0, SIGUSR1);
	pid_t p = 0;
	while (p >= 0)
	{
		p = waitpid(-1, NULL, WNOHANG);
	}
}

/* ====================================================================================================
* Function    :  discardShm()
* Definition  :  Detach and remove any shared memory.
* Parameter   :  Shared memory ID, shared memory address, shared memory name, current executable name,
                  and current process type.
* Return      :  None.
==================================================================================================== */
void discardShm(int shmid, void *shmaddr, char *shm_name, char *exe_name, char *process_type)
{
	//Detaching...
	if (shmaddr != NULL)
	{
		if ((shmdt(shmaddr)) << 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not detach [%s] shared memory!\n", exe_name, process_type, shm_name);
		}
	}

	//Deleting...
	if (shmid > 0)
	{
		if ((shmctl(shmid, IPC_RMID, NULL)) < 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not delete [%s] shared memory! Exiting...\n", exe_name, process_type, shm_name);
		}
	}
}

/* ====================================================================================================
* Function    :  cleanUp()
* Definition  :  Release all shared memory and delete all message queue, shared memory, and semaphore.
* Parameter   :  None.
* Return      :  None.
==================================================================================================== */
void cleanUp()
{
	//Delete [message queue] shared memory
	if (mqueueid > 0)
	{
		msgctl(mqueueid, IPC_RMID, NULL);
	}

	//Release and delete [shmclock] shared memory
	discardShm(shmclock_shmid, shmclock_shmptr, "shmclock", exe_name, "Master");

	//Delete semaphore
	if (semid > 0)
	{
		semctl(semid, 0, IPC_RMID);
	}

	//Release and delete [pcbt] shared memory
	discardShm(pcbt_shmid, pcbt_shmptr, "pcbt", exe_name, "Master");
}

/* ====================================================================================================
* Function    :  semaLock()
* Definition  :  Invoke semaphore lock of the given semaphore and index.
* Parameter   :  Semaphore Index.
* Return      :  None.
==================================================================================================== */
void semaLock(int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = -1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

/* ====================================================================================================
* Function    :  semaRelease()
* Definition  :  Release semaphore lock of the given semaphore and index.
* Parameter   :  Semaphore Index.
* Return      :  None.
==================================================================================================== */
void semaRelease(int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = 1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

void incShmclock()
{
	semaLock(0);
	int r_nano = rand() % 1000000 + 1;

	forkclock.ns += r_nano;
	shmclock_shmptr->ns += r_nano;

	if (shmclock_shmptr->ns >= 1000000000)
	{
		shmclock_shmptr->s++;
		shmclock_shmptr->ns = 1000000000 - shmclock_shmptr->ns;
	}

	semaRelease(0);
}

void initResource(Data *data)
{
	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		data->init[i] = rand() % 10 + 1;
		data->resource[i] = data->init[i];
	}

	data->shared = (SHARED_RESOURCES_MAX == 0) ? 0 : rand() % (SHARED_RESOURCES_MAX - (SHARED_RESOURCES_MAX - SHARED_RESOURCES_MIN)) + SHARED_RESOURCES_MIN;
}

void displayResource(FILE *fpw, Data data)
{
	fprintf(stderr, "===Total Resource===\n<");
	fprintf(fpw, "===Total Resource===\n<");
	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		fprintf(stderr, "%2d", data.resource[i]);
		fprintf(fpw, "%2d", data.resource[i]);

		if (i < RESOURCES_MAX - 1)
		{
			fprintf(stderr, " | ");
			fprintf(fpw, " | ");
		}
	}
	fprintf(stderr, ">\n\n");
	fprintf(fpw, ">\n");

	fprintf(stderr, "Sharable Resources: %d\n\n", data.shared);
	fprintf(fpw, "Sharable Resources: %d\n", data.shared);
	fflush(fpw);
}

void initPCBT(PCB *pcbt)
{
	int i;
	for (i = 0; i < PROCESSES_MAX; i++)
	{
		pcbt[i].spid = -1;
		pcbt[i].pid = -1;
	}
}

void initPCB(PCB *pcb, int index, pid_t pid, Data data)
{
	pcb->spid = index;
	pcb->pid = pid;

	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		pcb->maximum[i] = rand() % (data.init[i] + 1);
		pcb->allocation[i] = 0;
		pcb->request[i] = 0;
		pcb->release[i] = 0;
	}
}

void setMatrix(PCB *pcbt, Queue *queue, int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int count)
{
	QueueNode next;
	next.next = queue->front;

	int i, j;
	int c_index = next.next->index;
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			maxm[i][j] = pcbt[c_index].maximum[j];
			allot[i][j] = pcbt[c_index].allocation[j];
		}

		if (next.next->next != NULL)
		{
			next.next = next.next->next;
			c_index = next.next->index;
		}
		else
		{
			next.next = NULL;
		}
	}
}

void calculateNeedMatrix(Data *data, int need[][RESOURCES_MAX], int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int count)
{
	int i, j;
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			need[i][j] = maxm[i][j] - allot[i][j];
		}
	}
}

void displayVector(FILE *fpw, int *line_count, char *v_name, char *l_name, int vector[RESOURCES_MAX])
{
	fprintf(stderr, "===%s Resource===\n%3s :  <", v_name, l_name);
	fprintf(fpw, "===%s Resource===\n%3s :  <", v_name, l_name);

	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		fprintf(stderr, "%2d", vector[i]);
		fprintf(fpw, "%2d", vector[i]);

		if (i < RESOURCES_MAX - 1)
		{
			fprintf(stderr, " | ");
			fprintf(fpw, " | ");
		}
	}
	fprintf(stderr, ">\n");
	fprintf(fpw, ">\n");
	fflush(fpw);
}

void displayMatrix(FILE *fpw, int *line_count, char *m_name, Queue *queue, int matrix[][RESOURCES_MAX], int count)
{
	QueueNode next;
	next.next = queue->front;

	int i, j;
	fprintf(stderr, "===%s Matrix===\n", m_name);
	fprintf(fpw, "===%s Matrix===\n", m_name);

	for (i = 0; i < count; i++)
	{
		fprintf(stderr, "P%2d :  <", next.next->index);
		fprintf(fpw, "P%2d :  <", next.next->index);
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			fprintf(stderr, "%2d", matrix[i][j]);
			fprintf(fpw, "%2d", matrix[i][j]);

			if (j < RESOURCES_MAX - 1)
			{
				fprintf(stderr, " | ");
				fprintf(fpw, " | ");
			}
		}
		fprintf(stderr, ">\n");
		fprintf(fpw, ">\n");
		fflush(fpw);

		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}
}

bool bankerAlgorithm(FILE *fpw, int *line_count, bool verbose, Data *data, PCB *pcbt, Queue *queue, int c_index) {
	int i, p, j, k;

	//=====Check for null queue=====
	//Return true (safe) when working queue is null
	QueueNode next;
	next.next = queue->front;
	if (next.next == NULL)
	{
		return true;
	}

	//=====Initialization/Get information=====
	int count = queue_size(queue);
	int maxm[count][RESOURCES_MAX];
	int allot[count][RESOURCES_MAX];
	int req[RESOURCES_MAX];
	int need[count][RESOURCES_MAX];
	int avail[RESOURCES_MAX];

	//Setting up matrix base on given queue and process control block table
	setMatrix(pcbt, queue, maxm, allot, count);

	//Calculate need matrix
	calculateNeedMatrix(data, need, maxm, allot, count);

	//Setting up available vector and request vector
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		avail[i] = data->resource[i];
		req[i] = pcbt[c_index].request[i];
	}

	//Update available vector
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			avail[j] = avail[j] - allot[i][j];
		}
	}

	//Map the PID Index to NEED vector index
	int idx = 0;
	next.next = queue->front;
	while (next.next != NULL)
	{
		if (next.next->index == c_index)
		{
			break;
		}
		idx++;

		//Point the pointer to the next queue element
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}
	//DEBUG fprintf(fpw, "NEED index: %d\n", idx);

	//Display information
	if (verbose)
	{
		displayMatrix(fpw, line_count, "Maximum", queue, maxm, count);
		displayMatrix(fpw, line_count, "Allocation", queue, allot, count);
		char str[BUFFER_LENGTH];
		sprintf(str, "P%2d", c_index);
		displayVector(fpw, line_count, "Request", str, req);
	}

	//=====Finding SAFE Sequence=====
	bool finish[count];							  //To store finish
	int safeSeq[count];							  //To store safe sequence
	memset(finish, 0, count * sizeof(finish[0])); //Mark all processes as not finish

	//Make a copy of available resources (working vector)
	int work[RESOURCES_MAX];
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		work[i] = avail[i];
	}

	/* =====Resource Request Algorithm===== */
	for (j = 0; j < RESOURCES_MAX; j++)
	{
		//Check to see if the process is not asking for more than it will ever need
		if (need[idx][j] < req[j] && j < data->shared)
		{
			fprintf(stderr, "NOTICE: Asked for more than initial max request.\n");
			fprintf(fpw, "NOTICE: Asked for more than initial max request.\n");
			fflush(fpw);

			//Display information
			if (verbose)
			{
				displayVector(fpw, line_count, "Available", "A  ", avail);
				displayMatrix(fpw, line_count, "Need", queue, need, count);
			}
			return false;
		}

		if (req[j] <= avail[j] && j < data->shared)
		{
			avail[j] -= req[j];
			allot[idx][j] += req[j];
			need[idx][j] -= req[j];
		}
		else
		{
			fprintf(stderr, "NOTICE: Not enough available resources!\n");
			fprintf(fpw, "NOTICE: Not enough available resources!\n");
			fflush(fpw);

			//Display information
			if (verbose)
			{
				displayVector(fpw, line_count, "Available", "A  ", avail);
				displayMatrix(fpw, line_count, "Need", queue, need, count);
			}
			return false;
		}
	}

	/* =====Safety Algorithm===== */
	//While all processes are not finished or system is not in safe state.
	int index = 0;
	while (index < count)
	{
		//Find a process which is not finish and whose needs can be satisfied with current work[] resources.
		bool found = false;
		for (p = 0; p < count; p++)
		{
			//First check if a process is finished, if no, go for next condition
			if (finish[p] == 0)
			{
				//Check if for all resources of current process need is less than work
				for (j = 0; j < RESOURCES_MAX; j++)
				{
					if (need[p][j] > work[j] && data->shared)
					{
						break;
					}
				}

				//If all needs of p were satisfied.
				if (j == RESOURCES_MAX)
				{
					//Add the allocated resources of current process to the available/work resources i.e. free the resources
					for (k = 0; k < RESOURCES_MAX; k++)
					{
						work[k] += allot[p][k];
					}

					//Add this process to safe sequence.
					safeSeq[index++] = p;

					//Mark this p as finished and found is true
					finish[p] = 1;
					found = true;
				}
			}
		}

		//If we could not find a next process in safe sequence.
		if (found == false)
		{
			fprintf(stderr, "System is in UNSAFE (not safe) state\n");
			fprintf(fpw, "System is in UNSAFE (not safe) state\n");
			fflush(fpw);
			return false;
		}
	} //END OF: index < count

	//Display information
	if (verbose)
	{
		displayVector(fpw, line_count, "Available", "A  ", avail);
		displayMatrix(fpw, line_count, "Need", queue, need, count);
	}

	//Map the safe sequence with the queue sequence
	int sequence[count];
	int seq_index = 0;
	next.next = queue->front;
	while (next.next != NULL)
	{
		sequence[seq_index++] = next.next->index;

		//Point the pointer to the next queue element
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}

	//If system is in safe state then safe sequence will be as below
	fprintf(stderr, "System is in SAFE state. Safe sequence is: ");
	fprintf(fpw, "System is in SAFE state. Safe sequence is: ");
	for (i = 0; i < count; i++)
	{
		fprintf(stderr, "%2d ", sequence[safeSeq[i]]);
		fprintf(fpw, "%2d ", sequence[safeSeq[i]]);
	}
	fprintf(stderr, "\n\n");
	fprintf(fpw, "\n");
	fflush(fpw);

	return true;
}