/*
 * user.c November 24, 2020
 * Jared Diehl (jmddnb@umsystem.edu)
 */

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "shared.h"

static char *programName;

static char *exe_name;
static int exe_index;
static key_t key;

static int shmid = -1;
static int msqid = -1;
static System *system = NULL;
static Message message;

void registerSignalHandlers();
void signalHandler(int);
void initIPC();

#define SIZE 32

int main(int argc, char *argv[]) {
	registerSignalHandlers();

	exe_name = argv[0];
	exe_index = atoi(argv[1]);
	int m = atoi(argv[2]);
	srand(getpid());

	initIPC();

	bool is_terminate = false;
	int memory_reference = 0;
	unsigned int address = 0;
	unsigned int request_page = 0;
	while (true) {
		msgrcv(msqid, &message, (sizeof(Message) - sizeof(long)), getpid(), 0);

		if (memory_reference <= 1000) {
			if (m == 0)
			{
				address = rand() % 32768 + 0;
				request_page = address >> 10;
			} else {
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
		} else is_terminate = true;

		message.type = 1;
		message.flag = (is_terminate) ? 0 : 1;
		message.address = address;
		message.page = request_page;
		msgsnd(msqid, &message, (sizeof(Message) - sizeof(long)), 0);

		if (is_terminate) break;
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

void initIPC() {
	key_t key;

	if ((key = ftok(".", 0)) == -1) crash("ftok");
	if ((shmid = shmget(key, sizeof(System), 0)) == -1) crash("shmget");
	if ((system = (System*) shmat(shmid, NULL, 0)) == (void*) -1) crash("shmat");

	if ((key = ftok(".", 1)) == -1) crash("ftok");
	if ((msqid = msgget(key, 0)) == -1) crash("msgget");
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