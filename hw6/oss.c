/*
 * oss.c November 24, 2020
 * Jared Diehl (jmddnb@umsystem.edu)
 */

#include <errno.h>
#include <libgen.h>
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
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "list.h"
#include "queue.h"
#include "shared.h"

#define log _log

/* Simulation functions */
void initSystem();
void simulate();
void handleProcesses();
void trySpawnProcess();
void spawnProcess(int);
void initPCB(pid_t, int);
int findAvailablePID();
int advanceClock(int);

/* Program lifecycle functions */
void init(int, char**);
void usage(int);
void registerSignalHandlers();
void signalHandler(int);
void timer(int);
void initIPC();
void freeIPC();

/* Utility functions */
void error(char*, ...);
void crash(char*);
void log(char*, ...);
void flog(char*, ...);
void semLock(const int);
void semUnlock(const int);
void printSummary();

static char *programName;
static volatile bool quit = false;

/* IPC variables */
static int shmid = -1;
static int msqid = -1;
static int semid = -1;
static System *system = NULL;
static Message message;

/* Simulation variables */
static int scheme = SIMPLE;
static Queue *queue; /* Process queue */
static List *reference; /* Reference string */
static List *stack; /* LRU stack */
static Time nextSpawn;
static int activeCount = 0;
static int spawnCount = 0;
static int exitCount = 0;
static pid_t pids[PROCESSES_MAX];
static int memory[MAX_FRAMES];
static int memoryAccessCount = 0;
static int pageFaultCount = 0;
static unsigned int totalAccessTime = 0;

int main(int argc, char *argv[]) {
	init(argc, argv);

	srand(time(NULL) ^ getpid());

	bool ok = true;

	/* Get program arguments */
	while (true) {
		int c = getopt(argc, argv, "hm:");
		if (c == -1) break;
		switch (c) {
			case 'h':
				usage(EXIT_SUCCESS);
			case 'm':
				scheme = atoi(optarg);
				break;
			default:
				ok = false;
		}
	}

	/* Check for unknown arguments */
	if (optind < argc) {
		char buf[BUFFER_LENGTH];
		snprintf(buf, BUFFER_LENGTH, "found non-option(s): ");
		while (optind < argc) {
			strncat(buf, argv[optind++], BUFFER_LENGTH);
			if (optind < argc) strncat(buf, ", ", BUFFER_LENGTH);
		}
		error(buf);
		ok = false;
	}
	
	if (!ok) usage(EXIT_FAILURE);

	registerSignalHandlers();

	/* Clear log file */
	FILE *fp;
	if ((fp = fopen(PATH_LOG, "w")) == NULL) crash("fopen");
	if (fclose(fp) == EOF) crash("fclose");

	/* Setup simulation */
	initIPC();
	memset(pids, 0, sizeof(pids));
	system->clock.s = 0;
	system->clock.ns = 0;
	nextSpawn.s = 0;
	nextSpawn.ns = 0;
	initSystem();
	queue = queue_create();
	reference = list_create();
	stack = list_create();

	/* Start simulating */
	simulate();

	printSummary();

	/* Cleanup resources */
	freeIPC();

	return ok ? EXIT_SUCCESS  : EXIT_FAILURE;
}

void initSystem() {
	int i, j;

	/* Set default values in system data structures */
	for (i = 0; i < PROCESSES_MAX; i++) {
		system->ptable[i].pid = -1;
		system->ptable[i].spid = -1;
		for (j = 0; j < MAX_PAGES; j++) {
			system->ptable[i].ptable[j].frame = -1;
			system->ptable[i].ptable[j].protection = rand() % 2;
			system->ptable[i].ptable[j].dirty = 0;
			system->ptable[i].ptable[j].valid = 0;
		}
	}
}

/* Simulation driver */
void simulate() {
	/* Simulate run loop */
	while (true) {
		trySpawnProcess();
		advanceClock(0);
		handleProcesses();
		advanceClock(0);

		/* Catch an exited user process */
		int status;
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid > 0) {
			int spid = WEXITSTATUS(status);
			pids[spid] = 0;
			activeCount--;
			exitCount++;
		}

		/* Stop simulating if the last user process has exited */
		if (quit) {
			if (exitCount == spawnCount) break;
		} else {
			if (exitCount == PROCESSES_TOTAL) break;
		}
	}
}

/* Sends and receives messages from user processes, and acts upon them */
void handleProcesses() {
	QueueNode *next = queue->front;
	Queue *temp = queue_create();

	/* While we have user processes to simulate */
	while (next != NULL) {
		advanceClock(0);

		/* Send a message to a user process saying it's your turn to "run" */
		int spid = next->index;
		message.type = system->ptable[spid].pid;
		message.spid = spid;
		message.pid = system->ptable[spid].pid;
		msgsnd(msqid, &message, sizeof(Message), 0);

		/* Receive a response of what they're doing */
		msgrcv(msqid, &message, sizeof(Message), 1, 0);

		advanceClock(0);

		if (message.terminate) {
			flog("p%d terminated\n", message.spid);

			/* Free process' frames */
			int i;
			for (i = 0; i < MAX_PAGES; i++) {
				if (system->ptable[spid].ptable[i].frame != -1) {
					int frame = system->ptable[spid].ptable[i].frame;
					list_remove(reference, spid, i, frame);
					memory[frame / 8] &= ~(1 << (frame % 8));
				}
			}
		} else {
			int previousFrame;
			totalAccessTime += advanceClock(0);
			queue_push(temp, spid);

			unsigned int requestedAddress = message.address;
			unsigned int requestedPage = message.page;
			if (system->ptable[spid].ptable[requestedPage].protection == 0) {
				flog("p%d requesting address read at %d-%d\n", message.spid, requestedAddress, requestedPage);
			} else {
				flog("p%d requesting address write at %d-%d\n", message.spid, requestedAddress, requestedPage);
			}
			memoryAccessCount++;

			if (system->ptable[spid].ptable[requestedPage].valid == 0) {
				flog("p%d segfaulted at %d-%d\n", spid, requestedAddress, requestedPage);

				pageFaultCount++;

				totalAccessTime += advanceClock(100 * 1000000);

				/* Find available frame */
				bool isMemoryOpen = false;
				int frameCount = 0;
				while (true) {
					previousFrame = (previousFrame + 1) % MAX_FRAMES;
					if ((memory[previousFrame / 8] & (1 << (previousFrame % 8))) == 0) {
						isMemoryOpen = true;
						break;
					}
					if (frameCount++ >= MAX_FRAMES - 1) break;
				}

				/* Check if there is still space in memory */
				if (isMemoryOpen == true) {
					system->ptable[spid].ptable[requestedPage].frame = previousFrame;
					system->ptable[spid].ptable[requestedPage].valid = 1;

					memory[previousFrame / 8] |= (1 << (previousFrame % 8));

					list_add(reference, spid, requestedPage, previousFrame);
					flog("p%d allocated frame %d\n", message.spid, previousFrame);

					list_remove(stack, spid, requestedPage, previousFrame);
					list_add(stack, spid, requestedPage, previousFrame);

					if (system->ptable[spid].ptable[requestedPage].protection == 0) {
						flog("p%d given data from frame %d at %d-%d\n", spid, system->ptable[spid].ptable[requestedPage].frame, requestedAddress, requestedPage);
						system->ptable[spid].ptable[requestedPage].dirty = 0;
					} else {
						flog("p%d writing data to frame %d at %d-%d\n", spid, system->ptable[spid].ptable[requestedPage].frame, requestedAddress, requestedPage);
						system->ptable[spid].ptable[requestedPage].dirty = 1;
					}
				} else {
					/* Handle when memory is full */

					flog("%d-%d is not in frame\n", requestedAddress, requestedPage);

					unsigned int index = stack->head->index;
					unsigned int page = stack->head->page;
					unsigned int address = page << 10;
					unsigned int frame = stack->head->frame;

					if (system->ptable[index].ptable[page].dirty == 1) {
						/* Modified information is written back to disk */
						flog("%d-%d modified\n", address, page);
					}

					/* Page replacement */
					system->ptable[index].ptable[page].frame = -1;
					system->ptable[index].ptable[page].dirty = 0;
					system->ptable[index].ptable[page].valid = 0;
					system->ptable[spid].ptable[requestedPage].frame = frame;
					system->ptable[spid].ptable[requestedPage].dirty = 0;
					system->ptable[spid].ptable[requestedPage].valid = 1;
					list_remove(stack, index, page, frame);
					list_remove(reference, index, page, frame);
					list_add(stack, spid, requestedPage, frame);
					list_add(reference, spid, requestedPage, frame);

					if (system->ptable[spid].ptable[requestedPage].protection == 1) {
						log("%s: dirty bit of frame (%d) set, adding additional time to the clock\n", programName, previousFrame);
						log("%s: indicating to process (%d) [%d] that write has happend to address (%d) [%d]\n", programName, message.spid, message.pid, address, requestedPage);
						system->ptable[spid].ptable[requestedPage].dirty = 1;
					}
				}
			} else {
				int frame = system->ptable[spid].ptable[requestedPage].frame;
				list_remove(stack, spid, requestedPage, frame);
				list_add(stack, spid, requestedPage, frame);

				if (system->ptable[spid].ptable[requestedPage].protection == 0) {
					flog("%d-%d in frame %d, giving data to p%d\n", requestedAddress, requestedPage, system->ptable[spid].ptable[requestedPage].frame, message.spid);
				} else {
					flog("%d-%d in frame %d, writing data to it\n", requestedAddress, requestedPage, system->ptable[spid].ptable[requestedPage].frame);
				}
			}
		}

		/* Reset message */
		message.type = -1;
		message.spid = -1;
		message.pid = -1;
		message.terminate = false;
		message.page = -1;

		/* On to the next user process to simulate */
		next = (next->next != NULL) ? next->next : NULL;
	}

	/* Reset the current queue */
	while (!queue_empty(queue))
		queue_pop(queue);
	while (!queue_empty(temp)) {
		int i = temp->front->index;
		queue_push(queue, i);
		queue_pop(temp);
	}

	free(temp);
}

/* Attempts to spawn a new user process, but depends on the simulation's current state */
void trySpawnProcess() {
	/* Guard statements checking if we can even attempt to spawn a user process */
	if (activeCount >= PROCESSES_MAX) return;
	if (spawnCount >= PROCESSES_TOTAL) return;
	if (nextSpawn.ns < (rand() % (500 + 1)) * (1000000 + 1)) return;
	if (quit) return;

	/* Reset next spawn time */
	nextSpawn.ns = 0;

	/* Try to find an available simulated PID */
	int spid = findAvailablePID();

	/* Guard statement checking if we did find a PID */
	if (spid == -1) return;

	/* Now we can spawn a simulated user process */
	spawnProcess(spid);
}

void spawnProcess(int spid) {
	/* Fork a new user process */
	pid_t pid = fork();
	
	/* Record its PID */
	pids[spid] = pid;

	if (pid == -1) crash("fork");
	else if (pid == 0) {
		/* Since child, execute a new user process */
		char arg0[BUFFER_LENGTH];
		char arg1[BUFFER_LENGTH];
		sprintf(arg0, "%d", spid);
		sprintf(arg1, "%d", scheme);
		execl("./user", "user", arg0, arg1, (char*) NULL);
		crash("execl");
	}

	/* Since parent, initialize the new user process for simulation */
	initPCB(pid, spid);
	queue_push(queue, spid);
	activeCount++;
	spawnCount++;

	flog("p%d created\n", spid);
}

void initPCB(pid_t pid, int spid) {
	int i;

	/* Set default values in a user process' data structure */
	PCB *pcb = &system->ptable[spid];
	pcb->pid = pid;
	pcb->spid = spid;
	for (i = 0; i < MAX_PAGES; i++) {
		pcb->ptable[i].frame = -1;
		pcb->ptable[i].protection = rand() % 2;
		pcb->ptable[i].dirty = 0;
		pcb->ptable[i].valid = 0;
	}
}

/* Returns values [0-PROCESSES_MAX] for a found available PID, otherwise -1 for not found */
int findAvailablePID() {
	int i;
	for (i = 0; i < PROCESSES_MAX; i++)
		if (pids[i] == 0) return i;
	return -1;
}

int advanceClock(int ns) {
	int r = (ns > 0) ? ns : rand() % (1 * 1000) + 1;
	
	/* Increment system clock by random nanoseconds */
	semLock(0);
	nextSpawn.ns += r;
	system->clock.ns += r;
	while (system->clock.ns >= (1000 * 1000000)) {
		system->clock.s++;
		system->clock.ns -= (1000 * 1000000);
	}
	semUnlock(0);

	return r;
}

void init(int argc, char **argv)
{
	programName = argv[0];

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void usage(int status) {
	if (status != EXIT_SUCCESS) fprintf(stderr, "Try '%s -h' for more information\n", programName);
	else {
		printf("Usage: %s [-v]\n", programName);
		printf("    -v : verbose on\n");
	}
	exit(status);
}

void registerSignalHandlers() {
	struct sigaction sa;

	/* Set up SIGINT handler */
	if (sigemptyset(&sa.sa_mask) == -1) crash("sigemptyset");
	sa.sa_handler = &signalHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa, NULL) == -1) crash("sigaction");

	/* Set up SIGALRM handler */
	if (sigemptyset(&sa.sa_mask) == -1) crash("sigemptyset");
	sa.sa_handler = &signalHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa, NULL) == -1) crash("sigaction");

	/* Initialize timout timer */
	timer(TIMEOUT);

	signal(SIGSEGV, signalHandler);
}

void signalHandler(int sig) {
	if (sig == SIGALRM) quit = true;
	else {
		printSummary();

		/* Kill all running user processes */
		int i;
		for (i = 0; i < PROCESSES_MAX; i++)
			if (pids[i] > 0) kill(pids[i], SIGTERM);
		while (wait(NULL) > 0);

		freeIPC();
		exit(EXIT_SUCCESS);
	}
}

void timer(int duration) {
	struct itimerval val;
	val.it_value.tv_sec = duration;
	val.it_value.tv_usec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &val, NULL) == -1) crash("setitimer");
}

void initIPC() {
	key_t key;

	if ((key = ftok(".", 0)) == -1) crash("ftok");
	if ((shmid = shmget(key, sizeof(System), IPC_EXCL | IPC_CREAT | PERMS)) == -1) crash("shmget");
	if ((system = (System*) shmat(shmid, NULL, 0)) == (void*) -1) crash("shmat");

	if ((key = ftok(".", 1)) == -1) crash("ftok");
	if ((msqid = msgget(key, IPC_EXCL | IPC_CREAT | PERMS)) == -1) crash("msgget");

	if ((key = ftok(".", 2)) == -1) crash("ftok");
	if ((semid = semget(key, 1, IPC_EXCL | IPC_CREAT | PERMS)) == -1) crash("semget");
	if (semctl(semid, 0, SETVAL, 1) == -1) crash("semctl");
}

void freeIPC() {
	if (system != NULL && shmdt(system) == -1) crash("shmdt");
	if (shmid > 0 && shmctl(shmid, IPC_RMID, NULL) == -1) crash("shmdt");

	if (msqid > 0 && msgctl(msqid, IPC_RMID, NULL) == -1) crash("msgctl");

	if (semid > 0 && semctl(semid, 0, IPC_RMID) == -1) crash("semctl");
}

void error(char *fmt, ...) {
	char buf[BUFFER_LENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, BUFFER_LENGTH, fmt, args);
	va_end(args);
	
	fprintf(stderr, "%s: %s\n", programName, buf);
	
	freeIPC();
}

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	freeIPC();
	
	exit(EXIT_FAILURE);
}

void log(char *fmt, ...) {
	FILE *fp = fopen(PATH_LOG, "a+");
	if(fp == NULL) crash("fopen");

	char buf[BUFFER_LENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, BUFFER_LENGTH, fmt, args);
	va_end(args);

	fprintf(stderr, buf);
	fprintf(fp, buf);

	if (fclose(fp) == EOF) crash("fclose");
}

void flog(char *fmt, ...) {
	FILE *fp = fopen(PATH_LOG, "a+");
	if (fp == NULL) crash("fopen");
	
	char buf[BUFFER_LENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, BUFFER_LENGTH, fmt, args);
	va_end(args);
	
	char buff[BUFFER_LENGTH];
	snprintf(buff, BUFFER_LENGTH, "%s: [%d.%d] %s", basename(programName), system->clock.s, system->clock.ns, buf);
	
	fprintf(stderr, buff);
	fprintf(fp, buff);
	
	fclose(fp);
}

void semLock(const int index) {
	struct sembuf sop = { index, -1, 0 };
	if (semop(semid, &sop, 1) == -1) crash("semop");
}

void semUnlock(const int index) {
	struct sembuf sop = { index, 1, 0 };
	if (semop(semid, &sop, 1) == -1) crash("semop");
}

void printSummary() {
	double memoryAccessesPerSecond = (double) memoryAccessCount / (double) system->clock.s;
	double pageFaultsPerMemoryAccess = (double) pageFaultCount / (double) memoryAccessCount;
	double averageMemoryAccessSpeed = ((double) totalAccessTime / (double) memoryAccessCount) / (double) 1000000;

	log("\nSUMMARY\n");
	log("Total processes executed: %d %d %d\n", activeCount, spawnCount, exitCount);
	log("System time: %d.%d\n", system->clock.s, system->clock.ns);
	log("Memory access count: %d\n", memoryAccessCount);
	log("Memory accesses per second: %f\n", memoryAccessesPerSecond);
	log("Page fault count: %d\n", pageFaultCount);
	log("Page faults per memory access: %f\n", pageFaultsPerMemoryAccess);
	log("Average memory access speed: %f milliseconds\n", averageMemoryAccessSpeed);
	log("Total memory access time: %f milliseconds\n", (double) totalAccessTime / (double) 1000000);
}