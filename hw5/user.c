/*
 * user.c November 21, 2020
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

static char *exe_name;
static int exe_index;
static key_t key;

static int mqueueid = -1;
static Message user_message;
static int shmclock_shmid = -1;
static Time *shmclock_shmptr = NULL;
static int semid = -1;
static struct sembuf sema_operation;
static int pcbt_shmid = -1;
static PCB *pcbt_shmptr = NULL;

void processInterrupt();
void processHandler(int signum);
void resumeHandler(int signum);
void discardShm(void *shmaddr, char *shm_name, char *exe_name, char *process_type);
void cleanUp();
void semaLock(int sem_index);
void semaRelease(int sem_index);
void getSharedMemory();

void initIPC();

int main(int argc, char *argv[])
{
	processInterrupt();

	int i;
	exe_name = argv[0];
	exe_index = atoi(argv[1]);
	srand(getpid());

	getSharedMemory();

	bool is_resource_once = false;
	bool is_requesting = false;
	bool is_acquire = false;
	Time userStartClock;
	Time userEndClock;
	userStartClock.s = shmclock_shmptr->s;
	userStartClock.ns = shmclock_shmptr->ns;
	bool is_ran_duration = false;
	while (1)
	{
		msgrcv(mqueueid, &user_message, (sizeof(Message) - sizeof(long)), getpid(), 0);

		if (!is_ran_duration)
		{
			userEndClock.s = shmclock_shmptr->s;
			userEndClock.ns = shmclock_shmptr->ns;
			if (abs(userEndClock.ns - userStartClock.ns) >= 1000000000)
			{
				is_ran_duration = true;
			}
			else if (abs(userEndClock.s - userStartClock.s) >= 1)
			{
				is_ran_duration = true;
			}
		}

		bool is_terminate = false;
		bool is_releasing = false;
		int choice;
		if (!is_resource_once || !is_ran_duration)
		{
			choice = rand() % 2 + 0;
		}
		else
		{
			choice = rand() % 3 + 0;
		}

		if (choice == 0)
		{
			is_resource_once = true;

			if (!is_requesting)
			{
				for (i = 0; i < RESOURCES_MAX; i++)
				{
					pcbt_shmptr[exe_index].request[i] = rand() % (pcbt_shmptr[exe_index].maximum[i] - pcbt_shmptr[exe_index].allocation[i] + 1);
				}
				is_requesting = true;
			}
		}
		else if (choice == 1)
		{
			if (is_acquire)
			{
				for (i = 0; i < RESOURCES_MAX; i++)
				{
					pcbt_shmptr[exe_index].release[i] = pcbt_shmptr[exe_index].allocation[i];
				}
				is_releasing = true;
			}
		}
		else if (choice == 2)
		{
			is_terminate = true;
		}

		//Send a message to master that I got the signal and master should invoke an action base on my "choice"
		user_message.mtype = 1;
		user_message.flag = (is_terminate) ? 0 : 1;
		user_message.isRequest = (is_requesting) ? true : false;
		user_message.isRelease = (is_releasing) ? true : false;
		msgsnd(mqueueid, &user_message, (sizeof(Message) - sizeof(long)), 0);

		//--------------------------------------------------
		//Determine sleep again or end the current process
		//If sleep again, check if process can proceed the request OR release already allocated resources
		if (is_terminate)
		{
			break;
		}
		else
		{
			/* Update resources by either: 
			[1] If it requesting and it is SAFE, increment allocation vector. 
			[2] If it requesting and it is UNSAFE, do nothing.
			[3] If it releasing, decrement allocation vector base on current allocation vector */
			if (is_requesting)
			{
				//Waiting for master signal to determine if it safe to proceed the request
				msgrcv(mqueueid, &user_message, (sizeof(Message) - sizeof(long)), getpid(), 0);

				if (user_message.isSafe == true)
				{
					for (i = 0; i < RESOURCES_MAX; i++)
					{
						pcbt_shmptr[exe_index].allocation[i] += pcbt_shmptr[exe_index].request[i];
						pcbt_shmptr[exe_index].request[i] = 0;
					}
					is_requesting = false;
					is_acquire = true;
				}
			}

			if (is_releasing)
			{
				for (i = 0; i < RESOURCES_MAX; i++)
				{
					pcbt_shmptr[exe_index].allocation[i] -= pcbt_shmptr[exe_index].release[i];
					pcbt_shmptr[exe_index].release[i] = 0;
				}
				is_acquire = false;
			}
		}
	} //END OF: Infinite while loop #1

	cleanUp();
	exit(exe_index);
}

void processInterrupt()
{
	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &processHandler;
	sa1.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR1, &sa1, NULL) == -1)
	{
		perror("ERROR");
	}

	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &processHandler;
	sa2.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa2, NULL) == -1)
	{
		perror("ERROR");
	}
}
void processHandler(int signum)
{
//	printf("%d: Terminated!\n", getpid());
	cleanUp();
	exit(2);
}

void discardShm(void *shmaddr, char *shm_name, char *exe_name, char *process_type)
{
	//Detaching...
	if (shmaddr != NULL)
	{
		if ((shmdt(shmaddr)) << 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not detach [%s] shared memory!\n", exe_name, process_type, shm_name);
		}
	}
}

void cleanUp()
{
	//Release [shmclock] shared memory
	discardShm(shmclock_shmptr, "shmclock", exe_name, "Child");

	//Release [pcbt] shared memory
	discardShm(pcbt_shmptr, "pcbt", exe_name, "Child");
}

void semaLock(int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = -1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

void semaRelease(int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = 1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

void getSharedMemory()
{
	key = ftok("./oss.c", 1);
	mqueueid = msgget(key, 0600);
	if (mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [message queue] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	key = ftok("./oss.c", 2);
	shmclock_shmid = shmget(key, sizeof(Time), 0600);
	if (shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	shmclock_shmptr = shmat(shmclock_shmid, NULL, 0);
	if (shmclock_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	key = ftok("./oss.c", 3);
	semid = semget(key, 1, 0600);
	if (semid == -1)
	{
		fprintf(stderr, "%s ERROR: fail to attach a private semaphore! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(PCB) * PROCESSES_MAX;
	pcbt_shmid = shmget(key, process_table_size, 0600);
	if (pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	pcbt_shmptr = shmat(pcbt_shmid, NULL, 0);
	if (pcbt_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}
}
