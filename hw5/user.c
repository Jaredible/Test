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

void init(int, char**);
void registerSignalHandlers();
void signalHandler(int);
void initIPC();
void crash(char*);

static char *programName;

/* IPC variables */
static int shmid = -1;
static int msqid = -1;
static System *system = NULL;
static Message message;

int main(int argc, char **argv) {
	init(argc, argv);

	registerSignalHandlers();

	int spid = atoi(argv[1]);

	srand(time(NULL) ^ getpid());

	initIPC();

	bool started = false;
	bool requesting = false;
	bool acquired = false;
	Time arrival;
	Time duration;
	arrival.s = system->clock.s;
	arrival.ns = system->clock.ns;
	bool old = false;
	int i;

	/* Decision loop */
	while (true) {
		/* Wait until we get a message from OSS telling us it's our turn to "run" */
		msgrcv(msqid, &message, sizeof(Message), getpid(), 0);

		/* Check if has run for long enough */
		if (!old) {
			duration.s = system->clock.s;
			duration.ns = system->clock.ns;
			if (abs(duration.ns - arrival.ns) >= 1000 * 1000000) old = true;
			else if (abs(duration.s - arrival.s) >= 1) old = true;
		}

		bool terminating = false;
		bool releasing = false;
		int choice;

		if (!started || !old) choice = rand() % 2 + 0;
		else choice = rand() % 3 + 0;

		/* Make a decision (i.e., terminate, request, or release) */
		switch (choice) {
			case 0:
				started = true;
				if (!requesting) {
					for (i = 0; i < RESOURCES_MAX; i++)
						system->ptable[spid].request[i] = rand() % (system->ptable[spid].maximum[i] - system->ptable[spid].allocation[i] + 1);
					requesting = true;
				}
				break;
			case 1:
				if (acquired) {
					for (i = 0; i < RESOURCES_MAX; i++)
						system->ptable[spid].release[i] = system->ptable[spid].allocation[i];
					terminating = true;
				}
				break;
			case 2:
				terminating = true;
				break;
		}

		/* Send that decision to OSS */
		message.type = 1;
		message.terminate = terminating ? 0 : 1;
		message.request = requesting ? true : false;
		message.release = terminating ? true : false;
		msgsnd(msqid, &message, sizeof(Message), 0);

		/* Act upon that decision */
		if (terminating) break;
		else {
			if (requesting) {
				/* Wait for OSS to determine if the system is safe or not */
				msgrcv(msqid, &message, sizeof(Message), getpid(), 0);

				if (message.safe) {
					for (i = 0; i < RESOURCES_MAX; i++) {
						system->ptable[spid].allocation[i] += system->ptable[spid].request[i];
						system->ptable[spid].request[i] = 0;
					}

					requesting = false;
					acquired = true;
				}
			}

			if (releasing) {
				for (i = 0; i < RESOURCES_MAX; i++) {
					system->ptable[spid].allocation[i] -= system->ptable[spid].release[i];
					system->ptable[spid].release[i] = 0;
				}
				acquired = false;
			}
		}
	}
	
	return spid;
}

void init(int argc, char **argv) {
	programName = argv[0];

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void registerSignalHandlers() {
	struct sigaction sa;

	if (sigemptyset(&sa.sa_mask) == -1) crash("sigemptyset");
	sa.sa_handler = &signalHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR1, &sa, NULL) == -1) crash("sigaction");

	if (sigemptyset(&sa.sa_mask) == -1) crash("sigemptyset");
	sa.sa_handler = &signalHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa, NULL) == -1) crash("sigaction");
}

void signalHandler(int sig) {
	exit(EXIT_FAILURE);
}

void initIPC() {
	key_t key;

	if ((key = ftok(KEY_PATHNAME, KEY_ID_SYSTEM)) == -1) crash("ftok");
	if ((shmid = shmget(key, sizeof(System), IPC_EXCL | IPC_CREAT | PERMS)) == -1) crash("shmget");
	if ((system = (System*) shmat(shmid, NULL, 0)) == (void*) -1) crash("shmat");

	if ((key = ftok(KEY_PATHNAME, KEY_ID_MESSAGE_QUEUE)) == -1) crash("ftok");
	if ((msqid = msgget(key, IPC_EXCL | IPC_CREAT | PERMS)) == -1) crash("msgget");
}

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	exit(EXIT_FAILURE);
}