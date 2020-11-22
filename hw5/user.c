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

static char *programName;

static int shmid = -1;
static int msqid = -1;
static System *system = NULL;
static Message message;

void init(int, char**);
void registerSignalHandlers();
void signalHandler(int);
void initIPC();
void crash(char*);

int main(int argc, char **argv) {
	init(argc, argv);

	registerSignalHandlers();

	int spid = atoi(argv[1]);
	printf("spid: %d\n", spid);

	srand(time(NULL) ^ getpid());

	initIPC();

	bool started = false;
	bool canTerminate = false;
	bool requesting = false;
	bool acquired = false;
	Time arrival;
	Time duration;
	arrival.s = system->clock.s;
	arrival.ns = system->clock.ns;
	int i;

	while (true) {
		msgrcv(msqid, &message, sizeof(Message), getpid(), 0);

		if (!canTerminate) {
			duration.s = system->clock.s;
			duration.ns = system->clock.ns;
			if (abs(duration.ns - arrival.ns) >= 1000 * 1000000) canTerminate = true;
			else if (abs(duration.s - arrival.s) >= 1) canTerminate = true;
		}

		bool terminating = false;
		bool releasing = false;
		int choice;

		if (!started || !canTerminate) choice = rand() % 2 + 0;
		else choice = rand() % 3 + 0;

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
					releasing = true;
				}
			case 2:
				terminating = true;
				break;
		}

		message.type = 1;
		message.terminate = terminating;
		message.request = requesting;
		message.release = releasing;
		msgsnd(msqid, &message, sizeof(Message), 0);

		if (terminating) break;
		else {
			if (requesting) {
				msgrcv(msqid, &message, sizeof(Message), getpid(), 0);

				if (message.safe == true) {
					for (i = 0; i < RESOURCES_MAX; i++)
					{
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

	exit(spid);
}

void init(int argc, char **argv) {
	programName = argv[0];

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void registerSignalHandlers() {
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &signalHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR1, &sa, NULL) == -1) crash("sigaction");

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &signalHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa, NULL) == -1) crash("sigaction");
}

void signalHandler(int sig) {
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

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	exit(EXIT_FAILURE);
}