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

void init(int, char**);
void initIPC();
void crash(char*);

static char *programName;

/* IPC variables */
static int shmid = -1;
static int msqid = -1;
static System *system = NULL;
static Message message;

int main(int argc, char *argv[]) {
	init(argc, argv);

	int spid = atoi(argv[1]);
	int scheme = atoi(argv[2]);

	srand(time(NULL) ^ getpid());

	initIPC();

	bool terminate = false;
	int referenceCount = 0;
	unsigned int address = 0;
	unsigned int page = 0;

	/* Decision loop */
	while (true) {
		/* Wait until we get a message from OSS telling us it's our turn to "run" */
		msgrcv(msqid, &message, sizeof(Message), getpid(), 0);

		/* Continue getting address if we haven't referenced to our limit (1000) */
		if (referenceCount <= 1000) {
			if (scheme == SIMPLE) {
				/* Execute simple scheme algorithm */

				address = rand() % 32768 + 0;
				page = address >> 10;
			} else if (scheme == WEIGHTED) {
				/* Execute weighted scheme algorithm */

				double weights[PAGE_COUNT];
				int i, j, p, r;
				double sum;

				for (i = 0; i < PAGE_COUNT; i++)
					weights[i] = 0;

				for (i = 0; i < PAGE_COUNT; i++) {
					sum = 0;
					for (j = 0; j <= i; j++)
						sum += 1 / (double) (j + 1);
					weights[i] = sum;
				}

				r = rand() % ((int) weights[PAGE_COUNT - 1] + 1);

				for (i = 0; i < PAGE_COUNT; i++)
					if (weights[i] > r) {
						p = i;
						break;
					}
				
				address = (p << 10) + (rand() % 1024);
				page = p;
			} else crash("Unknown scheme!");

			referenceCount++;
		} else terminate = true;

		/* Send our decision to OSS */
		message.type = 1;
		message.terminate = terminate;
		message.address = address;
		message.page = page;
		msgsnd(msqid, &message, sizeof(Message), 0);

		if (terminate) break;
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

	if ((key = ftok(KEY_PATHNAME, KEY_ID_SYSTEM)) == -1) crash("ftok");
	if ((shmid = shmget(key, sizeof(System), 0)) == -1) crash("shmget");
	if ((system = (System*) shmat(shmid, NULL, 0)) == (void*) -1) crash("shmat");

	if ((key = ftok(KEY_PATHNAME, KEY_ID_MESSAGE_QUEUE)) == -1) crash("ftok");
	if ((msqid = msgget(key, 0)) == -1) crash("msgget");
}

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	exit(EXIT_FAILURE);
}