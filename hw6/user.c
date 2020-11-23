/*
 * user.c November 25, 2020
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
#include <math.h>

#include "shared.h"

static char *programName;

static char *exe_name;
static int exe_index;
static key_t key;

static int mqueueid = -1;
static Message user_message;
static int shmclock_shmid = -1;
static Time *shmclock_shmptr = NULL;
static int pcbt_shmid = -1;
static PCB *pcbt_shmptr = NULL;

void registerSignalHandlers();
void signalHandler(int);
void initIPC();

#define SIZE 32

int main(int argc, char *argv[])
{
	/* =====Signal Handling====== */
	registerSignalHandlers();

	//--------------------------------------------------
	/* =====Initialize resources===== */
	exe_name = argv[0];
	exe_index = atoi(argv[1]);
	int m = atoi(argv[2]);
	srand(getpid());

	//--------------------------------------------------
	/* =====Getting shared memory===== */
	initIPC();

	//--------------------------------------------------
	bool is_terminate = false;
	int memory_reference = 0;
	unsigned int address = 0;
	unsigned int request_page = 0;
	while (1)
	{
		//Waiting for master signal to get resources
		msgrcv(mqueueid, &user_message, (sizeof(Message) - sizeof(long)), getpid(), 0);
		//DEBUG fprintf(stderr, "%s (%d): my index [%d]\n", exe_name, getpid(), user_message.index);

		if (memory_reference <= 1000)
		{
			//- Requesting Memory -//
			if (m == 0)
			{
				address = rand() % 32768 + 0;
				request_page = address >> 10;
			}
			else
			{
				double weights[SIZE];

				int i, j;

				for (i = 0; i < SIZE; i++)
				{
					weights[i] = 0;
				}

				double sum;
				for (i = 0; i < SIZE; i++)
				{
					sum = 0;
					for (j = 0; j <= i; j++)
					{
						sum += 1 / (double)(j + 1);
					}
					weights[i] = sum;
				}

				int r = rand() % ((int)weights[SIZE - 1] + 1);

				int page;
				for (i = 0; i < SIZE; i++)
				{
					if (weights[i] > r)
					{
						page = i;
						break;
					}
				}

				int offset = (page << 10) + (rand() % 1024);

				address = offset;
				request_page = page;
			}
			memory_reference++;
		}
		else
		{
			is_terminate = true;
		}

		//Send a message to master that I got the signal and master should invoke an action base on my data
		user_message.type = 1;
		user_message.flag = (is_terminate) ? 0 : 1;
		user_message.address = address;
		user_message.page = request_page;
		msgsnd(mqueueid, &user_message, (sizeof(Message) - sizeof(long)), 0);

		//--------------------------------------------------
		//Determine sleep again or end the current process
		if (is_terminate)
		{
			break;
		}
	}

	exit(exe_index);
}

void registerSignalHandlers()
{
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &signalHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR1, &sa, NULL) == -1)
	{
		perror("ERROR");
	}

	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &signalHandler;
	sa2.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa2, NULL) == -1)
	{
		perror("ERROR");
	}
}
void signalHandler(int sig)
{
	printf("%d: Terminated!\n", getpid());
	exit(2);
}

void initIPC()
{
	/* =====Getting [message queue] shared memory===== */
	key = ftok("./oss.c", 1);
	mqueueid = msgget(key, 0600);
	if (mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [message queue] shared memory! Exiting...\n", exe_name);
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Getting [shmclock] shared memory===== */
	key = ftok("./oss.c", 2);
	shmclock_shmid = shmget(key, sizeof(Time), 0600);
	if (shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [shmclock] shared memory! Exiting...\n", exe_name);
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it.
	shmclock_shmptr = shmat(shmclock_shmid, NULL, 0);
	if (shmclock_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", exe_name);
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Getting process control block table===== */
	key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(PCB) * MAX_PROCESS;
	pcbt_shmid = shmget(key, process_table_size, 0600);
	if (pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [pcbt] shared memory! Exiting...\n", exe_name);
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it.
	pcbt_shmptr = shmat(pcbt_shmid, NULL, 0);
	if (pcbt_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", exe_name);
		exit(EXIT_FAILURE);
	}
}

void init(int argc, char **argv) {
	programName = argv[0];

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	exit(EXIT_FAILURE);
}