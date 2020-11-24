/*
 * oss.c November 24, 2020
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

static pid_t pids[PROCESSES_MAX];

static char *programName;
static volatile bool quit = false;

static int scheme = 0;
static Queue *queue;
static Time nextSpawn;

static int shmid = -1;
static int msqid = -1;
static int semid = -1;
static System *system = NULL;
static Message message;

static int activeCount = 0;
static int spawnCount = 0;
static int exitCount = 0;

static int memoryaccess_number = 0;
static int pagefault_number = 0;
static unsigned int total_access_time = 0;
static unsigned char main_memory[MAX_FRAME];
static int last_frame = -1;
static List *reference_string;
static List *lru_stack;

int findAvailablePID();
void printSummary();

void initSystem();
void initDescriptor();
void simulate();
void handleProcesses();
void trySpawnProcess();
void spawnProcess(int);
void initPCB(pid_t, int);
int findAvailablePID();
int advanceClock(int);

void log(char*, ...);
void registerSignalHandlers();
void signalHandler(int);
void timer(int);
void finalize();
void semLock(const int);
void semUnlock(const int);

void init(int, char**);
void usage(int);
void registerSignalHandlers();
void signalHandler(int);
void timer(int);
void initIPC();
void freeIPC();
void finalize();
void error(char*, ...);
void crash(char*);

void initSystem();

void trySpawnProcess() {
	int spawn_nano = rand() % 500000000 + 1000000;
	if (activeCount < PROCESSES_MAX && spawnCount < PROCESSES_TOTAL && nextSpawn.ns >= spawn_nano && !quit) {
		nextSpawn.ns = 0;

		int spid = findAvailablePID();
		if (spid >= 0) spawnProcess(spid);
	}
}

void spawnProcess(int spid) {
	pid_t pid = fork();
	pids[spid] = pid;
	if (pid == -1) crash("fork");
	else if (pid == 0) {
		char arg0[BUFFER_LENGTH];
		char arg1[BUFFER_LENGTH];
		sprintf(arg0, "%d", spid);
		sprintf(arg1, "%d", scheme);
		execl("./user", "user", arg0, arg1, (char*) NULL);
		crash("execl");
	}

	activeCount++;
	spawnCount++;

	initPCB(pid, spid);

	queue_push(queue, spid);

	log("%s: generating process with PID (%d) [%d] and putting it in queue at time %d.%d\n", programName,
				system->ptable[spid].spid, system->ptable[spid].pid, system->clock.s, system->clock.ns);
}

void handleProcesses() {
	QueueNode *next;
	Queue *temp = queue_create();

	next = queue->front;
	while (next != NULL) {
		advanceClock(0);

		int index = next->index;
		message.type = system->ptable[index].pid;
		message.spid = index;
		message.pid = system->ptable[index].pid;
		msgsnd(msqid, &message, (sizeof(Message) - sizeof(long)), 0);

		msgrcv(msqid, &message, (sizeof(Message) - sizeof(long)), 1, 0);

		advanceClock(0);

		if (message.flag == 0) {
			log("%s: process with PID (%d) [%d] has finish running at my time %d.%d\n",
						programName, message.spid, message.pid, system->clock.s, system->clock.ns);

			int i;
			for (i = 0; i < MAX_PAGE; i++)
			{
				if (system->ptable[index].ptable[i].frame != -1)
				{
					int frame = system->ptable[index].ptable[i].frame;
					list_remove(reference_string, index, i, frame);
					main_memory[frame / 8] &= ~(1 << (frame % 8));
				}
			}
		} else {
			total_access_time += advanceClock(0);
			queue_push(temp, index);

			unsigned int address = message.address;
			unsigned int request_page = message.page;
			if (system->ptable[index].ptable[request_page].protection == 0) {
				log("%s: process (%d) [%d] requesting read of address (%d) [%d] at time %d:%d\n",
							programName, message.spid, message.pid,
							address, request_page,
							system->clock.s, system->clock.ns);
			} else {
				log("%s: process (%d) [%d] requesting write of address (%d) [%d] at time %d:%d\n",
							programName, message.spid, message.pid,
							address, request_page,
							system->clock.s, system->clock.ns);
			}
			memoryaccess_number++;

			if (system->ptable[index].ptable[request_page].valid == 0) {
				log("%s: address (%d) [%d] is not in a frame, PAGEFAULT\n",
							programName, address, request_page);
				pagefault_number++;

				total_access_time += advanceClock(14000000);

				bool is_memory_open = false;
				int count_frame = 0;
				while (true) {
					last_frame = (last_frame + 1) % MAX_FRAME;
					uint32_t frame = main_memory[last_frame / 8] & (1 << (last_frame % 8));
					if (frame == 0)
					{
						is_memory_open = true;
						break;
					}

					if (count_frame >= MAX_FRAME - 1)
					{
						break;
					}
					count_frame++;
				}

				if (is_memory_open == true) {
					system->ptable[index].ptable[request_page].frame = last_frame;
					system->ptable[index].ptable[request_page].valid = 1;

					main_memory[last_frame / 8] |= (1 << (last_frame % 8));

					list_add(reference_string, index, request_page, last_frame);
					log("%s: allocated frame [%d] to PID (%d) [%d]\n",
								programName, last_frame, message.spid, message.pid);

					list_remove(lru_stack, index, request_page, last_frame);
					list_add(lru_stack, index, request_page, last_frame);

					if (system->ptable[index].ptable[request_page].protection == 0) {
						log("%s: address (%d) [%d] in frame (%d), giving data to process (%d) [%d] at time %d:%d\n",
									programName, address, request_page,
									system->ptable[index].ptable[request_page].frame,
									message.spid, message.pid,
									system->clock.s, system->clock.ns);

						system->ptable[index].ptable[request_page].dirty = 0;
					} else {
						log("%s: address (%d) [%d] in frame (%d), writing data to frame at time %d:%d\n",
									programName, address, request_page,
									system->ptable[index].ptable[request_page].frame,
									system->clock.s, system->clock.ns);

						system->ptable[index].ptable[request_page].dirty = 1;
					}
				} else {
					log("%s: address (%d) [%d] is not in a frame, memory is full. Invoking page replacement...\n",
								programName, address, request_page);

					unsigned int lru_index = lru_stack->head->index;
					unsigned int lru_page = lru_stack->head->page;
					unsigned int lru_address = lru_page << 10;
					unsigned int lru_frame = lru_stack->head->frame;

					if (system->ptable[lru_index].ptable[lru_page].dirty == 1) {
						log("%s: address (%d) [%d] was modified. Modified information is written back to the disk\n",
									programName, lru_address, lru_page);
					}

					system->ptable[lru_index].ptable[lru_page].frame = -1;
					system->ptable[lru_index].ptable[lru_page].dirty = 0;
					system->ptable[lru_index].ptable[lru_page].valid = 0;

					system->ptable[index].ptable[request_page].frame = lru_frame;
					system->ptable[index].ptable[request_page].dirty = 0;
					system->ptable[index].ptable[request_page].valid = 1;

					list_remove(lru_stack, lru_index, lru_page, lru_frame);
					list_remove(reference_string, lru_index, lru_page, lru_frame);
					list_add(lru_stack, index, request_page, lru_frame);
					list_add(reference_string, index, request_page, lru_frame);

					if (system->ptable[index].ptable[request_page].protection == 1) {
						log("%s: dirty bit of frame (%d) set, adding additional time to the clock\n", programName, last_frame);
						log("%s: indicating to process (%d) [%d] that write has happend to address (%d) [%d]\n",
									programName, message.spid, message.pid, address, request_page);
						system->ptable[index].ptable[request_page].dirty = 1;
					}
				}
			} else {
				int frame = system->ptable[index].ptable[request_page].frame;
				list_remove(lru_stack, index, request_page, frame);
				list_add(lru_stack, index, request_page, frame);

				if (system->ptable[index].ptable[request_page].protection == 0) {
					log("%s: address (%d) [%d] is already in frame (%d), giving data to process (%d) [%d] at time %d:%d\n",
								programName, address, request_page,
								system->ptable[index].ptable[request_page].frame,
								message.spid, message.pid,
								system->clock.s, system->clock.ns);
				} else {
					log("%s: address (%d) [%d] is already in frame (%d), writing data to frame at time %d:%d\n",
								programName, address, request_page,
								system->ptable[index].ptable[request_page].frame,
								system->clock.s, system->clock.ns);
				}
			}
		}

		next = (next->next != NULL) ? next->next : NULL;

		message.type = -1;
		message.spid = -1;
		message.pid = -1;
		message.flag = -1;
		message.page = -1;
	}

	while (!queue_empty(queue))
		queue_pop(queue);
	while (!queue_empty(temp)) {
		int i = temp->front->index;
		queue_push(queue, i);
		queue_pop(temp);
	}

	free(temp);
}

void simulate() {
	while (true) {
		trySpawnProcess();
		advanceClock(0);
		handleProcesses();
		advanceClock(0);

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

int main(int argc, char *argv[]) {
	init(argc, argv);

	srand(time(NULL) ^ getpid());

	bool ok = true;

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
	reference_string = list_create();
	lru_stack = list_create();
	initDescriptor();
	printDescriptor();

	/* Start simulating */
	simulate();

	printSummary();

	/* Cleanup resources */
	freeIPC();

	return ok ? EXIT_SUCCESS  : EXIT_FAILURE;
}

void log(char *fmt, ...) {
	FILE *fp = fopen("output.log", "a+");
	if (fp == NULL) perror("ERROR");

	char buf[BUFFER_LENGTH];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	fprintf(stderr, buf);
	fprintf(fp, buf);

	if (fclose(fp) == EOF) perror("ERROR");
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

void finalize() {
	printSummary();

	freeIPC();

	kill(0, SIGUSR1);
	while (waitpid(-1, NULL, WNOHANG) >= 0);
}

void semLock(const int index) {
	struct sembuf sop = { index, -1, 0 };
	if (semop(semid, &sop, 1) == -1) crash("semop");
}

void semUnlock(const int index) {
	struct sembuf sop = { index, 1, 0 };
	if (semop(semid, &sop, 1) == -1) crash("semop");
}

int advanceClock(int ns) {
	semLock(0);

	/* Increment system clock by random nanoseconds */
	int r = (ns > 0) ? ns : rand() % (1 * 1000) + 1;
	nextSpawn.ns += r;
	system->clock.ns += r;
	while (system->clock.ns >= (1000 * 1000000)) {
		system->clock.s++;
		system->clock.ns -= (1000 * 1000000);
	}

	semUnlock(0);

	return r;
}

void initSystem()
{
	int i, j;
	for (i = 0; i < PROCESSES_MAX; i++)
	{
		system->ptable[i].spid = -1;
		system->ptable[i].pid = -1;
		for (j = 0; j < MAX_PAGE; j++)
		{
			system->ptable[i].ptable[j].frame = -1;
			system->ptable[i].ptable[j].protection = rand() % 2;
			system->ptable[i].ptable[j].dirty = 0;
			system->ptable[i].ptable[j].valid = 0;
		}
	}
}

void initPCB(pid_t pid, int spid) {
	int i;

	PCB *pcb = &system->ptable[spid];
	pcb->pid = pid;
	pcb->spid = spid;
	for (i = 0; i < MAX_PAGE; i++) {
		pcb->ptable[i].frame = -1;
		pcb->ptable[i].protection = rand() % 2;
		pcb->ptable[i].dirty = 0;
		pcb->ptable[i].valid = 0;
	}
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

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	freeIPC();
	
	exit(EXIT_FAILURE);
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

int findAvailablePID() {
	int i;
	for (i = 0; i < PROCESSES_MAX; i++)
		if (pids[i] == 0) return i;
	return -1;
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

void printSummary() {
	double mem_p_sec = (double)memoryaccess_number / (double)system->clock.s;
	double pg_f_p_mem = (double)pagefault_number / (double)memoryaccess_number;
	double avg_m = (double)total_access_time / (double)memoryaccess_number;
	avg_m /= 1000000.0;

	log("- Master PID: %d\n", getpid());
	log("- Number of forking during this execution: %d\n", spawnCount);
	log("- Final simulation time of this execution: %d.%d\n", system->clock.s, system->clock.ns);
	log("- Number of memory accesses: %d\n", memoryaccess_number);
	log("- Number of memory accesses per nanosecond: %f memory/second\n", mem_p_sec);
	log("- Number of page faults: %d\n", pagefault_number);
	log("- Number of page faults per memory access: %f pagefault/access\n", pg_f_p_mem);
	log("- Average memory access speed: %f ms/n\n", avg_m);
	log("- Total memory access time: %f ms\n", (double)total_access_time / 1000000.0);
	fprintf(stderr, "SIMULATION RESULT is recorded into the log file: %s\n", "output.log");
}