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

static int shmid = -1;
static int msqid = -1;
static System *system = NULL;
static Message message;

void init(int, char**);
void initIPC();
void crash(char*);

#define SIZE 32

int main(int argc, char *argv[]) {
	init(argc, argv);

	int spid = atoi(argv[1]);
	int scheme = atoi(argv[2]);

	srand(time(NULL) ^ getpid());

	initIPC();

	bool is_terminate = false;
	int referenceCount = 0;
	unsigned int address = 0;
	unsigned int page = 0;
	while (true) {
		msgrcv(msqid, &message, (sizeof(Message) - sizeof(long)), getpid(), 0);

		if (referenceCount <= 1000) {
			if (scheme == SIMPLE) {
				address = rand() % 32768 + 0;
				page = address >> 10;
			} else if (scheme == WEIGHTED) {
				double weights[SIZE];

				int i, j;

				for (i = 0; i < SIZE; i++)
					weights[i] = 0;

				double sum;
				for (i = 0; i < SIZE; i++) {
					sum = 0;
					for (j = 0; j <= i; j++)
						sum += 1 / (double)(j + 1);
					weights[i] = sum;
				}

				int r = rand() % ((int)weights[SIZE - 1] + 1);

				int p;
				for (i = 0; i < SIZE; i++)
					if (weights[i] > r) {
						p = i;
						break;
					}
				
				address = (p << 10) + (rand() % 1024);
				page = p;
			} else {
				crash("Unknown scheme!");
			}

			referenceCount++;
		} else is_terminate = true;

		message.type = 1;
		message.terminate = is_terminate;
		message.address = address;
		message.page = page;
		msgsnd(msqid, &message, (sizeof(Message) - sizeof(long)), 0);

		if (is_terminate) break;
	}

	return spid;
}

void init(int argc, char **argv) {
	programName = argv[0];

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void initIPC() {
	key_t key;

	if ((key = ftok(".", 0)) == -1) crash("ftok");
	if ((shmid = shmget(key, sizeof(System), 0)) == -1) crash("shmget");
	if ((system = (System*) shmat(shmid, NULL, 0)) == (void*) -1) crash("shmat");

	if ((key = ftok(".", 1)) == -1) crash("ftok");
	if ((msqid = msgget(key, 0)) == -1) crash("msgget");
}

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	exit(EXIT_FAILURE);
}