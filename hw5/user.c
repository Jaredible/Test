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

static int spid;

static int shmid = -1;
static int msqid = -1;

static System *system = NULL;
static Message message;

void registerSignalHandlers();
void signalHandler(int);

void initIPC();
void crash(char*);
void init(int, char**);

int main(int argc, char **argv) {
	init(argc, argv);

	registerSignalHandlers();

	int i;
	spid = atoi(argv[1]);

	srand(time(NULL) ^ getpid());

	initIPC();

	bool is_resource_once = false;
	bool is_requesting = false;
	bool is_acquire = false;
	Time userStartClock;
	Time userEndClock;
	userStartClock.s = system->clock.s;
	userStartClock.ns = system->clock.ns;
	bool is_ran_duration = false;

	while (true) {
		msgrcv(msqid, &message, sizeof(Message), getpid(), 0);

		if (!is_ran_duration)
		{
			userEndClock.s = system->clock.s;
			userEndClock.ns = system->clock.ns;
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
		if (!is_resource_once || !is_ran_duration) choice = rand() % 2 + 0;
		else choice = rand() % 3 + 0;

		if (choice == 0) {
			is_resource_once = true;

			if (!is_requesting) {
				for (i = 0; i < RESOURCES_MAX; i++) {
					system->ptable[spid].request[i] = rand() % (system->ptable[spid].maximum[i] - system->ptable[spid].allocation[i] + 1);
				}
				is_requesting = true;
			}
		}
		else if (choice == 1) {
			if (is_acquire) {
				for (i = 0; i < RESOURCES_MAX; i++) {
					system->ptable[spid].release[i] = system->ptable[spid].allocation[i];
				}
				is_releasing = true;
			}
		}
		else if (choice == 2) {
			is_terminate = true;
		}

		message.type = 1;
		message.terminate = is_terminate ? 0 : 1;
		message.request = is_requesting ? true : false;
		message.release = is_releasing ? true : false;
		msgsnd(msqid, &message, sizeof(Message), 0);

		if (is_terminate) break;
		else {
			if (is_requesting) {
				msgrcv(msqid, &message, sizeof(Message), getpid(), 0);

				if (message.safe == true) {
					for (i = 0; i < RESOURCES_MAX; i++)
					{
						system->ptable[spid].allocation[i] += system->ptable[spid].request[i];
						system->ptable[spid].request[i] = 0;
					}
					is_requesting = false;
					is_acquire = true;
				}
			}

			if (is_releasing) {
				for (i = 0; i < RESOURCES_MAX; i++) {
					system->ptable[spid].allocation[i] -= system->ptable[spid].release[i];
					system->ptable[spid].release[i] = 0;
				}
				is_acquire = false;
			}
		}
	}

	return spid;
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

void init(int argc, char **argv) {
	programName = argv[0];

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}