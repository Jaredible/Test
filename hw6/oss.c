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

static int scheme = 0;
static Queue *queue;
static Time nextSpawn;

static int shmid = -1;
static int msqid = -1;
static int semid = -1;
static System *system = NULL;
static Message message;

static int fork_number = 0;
static pid_t pid = -1;
static unsigned char bitmap[PROCESSES_MAX];

static int memoryaccess_number = 0;
static int pagefault_number = 0;
static unsigned int total_access_time = 0;
static unsigned char main_memory[MAX_FRAME];
static int last_frame = -1;
static List *reference_string;
static List *lru_stack;

void log(char*, ...);
void masterInterrupt(int);
void masterHandler(int);
void segHandler(int);
void timer(int);
void finalize();
void discardShm(int shmid, void *shmaddr, char *shm_name, char *exe_name, char *process_type);
void semLock(const int);
void semUnlock(const int);
int advanceClock(int);

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
void initPCB(PCB*, int, pid_t);

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

	initIPC();

	memset(pids, 0, sizeof(pids));

	system->clock.s = 0;
	system->clock.ns = 0;
	nextSpawn.s = 0;
	nextSpawn.ns = 0;

	FILE *fp;
	if ((fp = fopen(PATH_LOG, "w")) == NULL) crash("fopen");
	if (fclose(fp) == EOF) crash("fclose");

	initSystem();

	queue = queue_create();
	reference_string = list_create();
	lru_stack = list_create();

	masterInterrupt(TIMEOUT);

	fprintf(stderr, "Using Least Recently Use (LRU) algorithm.\n");

	int last_index = -1;
	while (1)
	{
		int spawn_nano = rand() % 500000000 + 1000000;
		if (nextSpawn.ns >= spawn_nano)
		{
			nextSpawn.ns = 0;

			bool is_bitmap_open = false;
			int count_process = 0;
			while (1)
			{
				last_index = (last_index + 1) % PROCESSES_MAX;
				uint32_t bit = bitmap[last_index / 8] & (1 << (last_index % 8));
				if (bit == 0)
				{
					is_bitmap_open = true;
					break;
				}

				if (count_process >= PROCESSES_MAX - 1)
				{
					break;
				}
				count_process++;
			}

			if (is_bitmap_open == true) {
				pid = fork();
				if (pid == -1) crash("fork");
				else if (pid == 0) {
					char arg0[BUFFER_LENGTH];
					char arg1[BUFFER_LENGTH];
					sprintf(arg0, "%d", last_index);
					sprintf(arg1, "%d", scheme);
					execl("./user", "user", arg0, arg1, (char*) NULL);
					crash("execl");
				}

				fork_number++;

				bitmap[last_index / 8] |= (1 << (last_index % 8));

				initPCB(&system->ptable[last_index], last_index, pid);

				queue_push(queue, last_index);

				log("%s: generating process with PID (%d) [%d] and putting it in queue at time %d.%d\n", programName,
							system->ptable[last_index].spid, system->ptable[last_index].pid, system->clock.s, system->clock.ns);
			}
		}

		advanceClock(0);

		QueueNode *next;
		Queue *temp = queue_create();

		next = queue->front;
		while (next != NULL)
		{
			advanceClock(0);

			int index = next->index;
			message.type = system->ptable[index].pid;
			message.spid = index;
			message.pid = system->ptable[index].pid;
			msgsnd(msqid, &message, (sizeof(Message) - sizeof(long)), 0);

			msgrcv(msqid, &message, (sizeof(Message) - sizeof(long)), 1, 0);

			advanceClock(0);

			if (message.flag == 0)
			{
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
			}
			else
			{
				total_access_time += advanceClock(0);
				queue_push(temp, index);

				unsigned int address = message.address;
				unsigned int request_page = message.page;
				if (system->ptable[index].ptable[request_page].protection == 0)
				{
					log("%s: process (%d) [%d] requesting read of address (%d) [%d] at time %d:%d\n",
							   programName, message.spid, message.pid,
							   address, request_page,
							   system->clock.s, system->clock.ns);
				}
				else
				{
					log("%s: process (%d) [%d] requesting write of address (%d) [%d] at time %d:%d\n",
							   programName, message.spid, message.pid,
							   address, request_page,
							   system->clock.s, system->clock.ns);
				}
				memoryaccess_number++;

				if (system->ptable[index].ptable[request_page].valid == 0)
				{
					log("%s: address (%d) [%d] is not in a frame, PAGEFAULT\n",
							   programName, address, request_page);
					pagefault_number++;

					total_access_time += advanceClock(14000000);

					bool is_memory_open = false;
					int count_frame = 0;
					while (1)
					{
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

					if (is_memory_open == true)
					{
						system->ptable[index].ptable[request_page].frame = last_frame;
						system->ptable[index].ptable[request_page].valid = 1;

						main_memory[last_frame / 8] |= (1 << (last_frame % 8));

						list_add(reference_string, index, request_page, last_frame);
						log("%s: allocated frame [%d] to PID (%d) [%d]\n",
								   programName, last_frame, message.spid, message.pid);

						list_remove(lru_stack, index, request_page, last_frame);
						list_add(lru_stack, index, request_page, last_frame);

						if (system->ptable[index].ptable[request_page].protection == 0)
						{
							log("%s: address (%d) [%d] in frame (%d), giving data to process (%d) [%d] at time %d:%d\n",
									   programName, address, request_page,
									   system->ptable[index].ptable[request_page].frame,
									   message.spid, message.pid,
									   system->clock.s, system->clock.ns);

							system->ptable[index].ptable[request_page].dirty = 0;
						}
						else
						{
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

						if (system->ptable[index].ptable[request_page].protection == 1)
						{
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

		advanceClock(0);

		int child_status = 0;
		pid_t child_pid = waitpid(-1, &child_status, WNOHANG);

		if (child_pid > 0) {
			int return_index = WEXITSTATUS(child_status);
			bitmap[return_index / 8] &= ~(1 << (return_index % 8));
		}

		if (fork_number >= PROCESSES_TOTAL) {
			timer(0);
			masterHandler(0);
		}
	}

	return EXIT_SUCCESS;
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

void masterInterrupt(int seconds) {
	timer(seconds);

	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &masterHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa, NULL) == -1) perror("ERROR");

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &masterHandler;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa, NULL) == -1) perror("ERROR");

	signal(SIGUSR1, SIG_IGN);

	signal(SIGSEGV, segHandler);
}

void masterHandler(int signum) {
	finalize();
	exit(EXIT_SUCCESS);
}

void segHandler(int signum) {
	fprintf(stderr, "Segmentation Fault\n");
	masterHandler(0);
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
	double mem_p_sec = (double)memoryaccess_number / (double)system->clock.s;
	double pg_f_p_mem = (double)pagefault_number / (double)memoryaccess_number;
	double avg_m = (double)total_access_time / (double)memoryaccess_number;
	avg_m /= 1000000.0;

	log("- Master PID: %d\n", getpid());
	log("- Number of forking during this execution: %d\n", fork_number);
	log("- Final simulation time of this execution: %d.%d\n", system->clock.s, system->clock.ns);
	log("- Number of memory accesses: %d\n", memoryaccess_number);
	log("- Number of memory accesses per nanosecond: %f memory/second\n", mem_p_sec);
	log("- Number of page faults: %d\n", pagefault_number);
	log("- Number of page faults per memory access: %f pagefault/access\n", pg_f_p_mem);
	log("- Average memory access speed: %f ms/n\n", avg_m);
	log("- Total memory access time: %f ms\n", (double)total_access_time / 1000000.0);
	fprintf(stderr, "SIMULATION RESULT is recorded into the log file: %s\n", "output.log");

	freeIPC();

	kill(0, SIGUSR1);
	while (waitpid(-1, NULL, WNOHANG) >= 0);
}

void discardShm(int shmid, void *shmaddr, char *shm_name, char *exe_name, char *process_type)
{
	if (shmaddr != NULL)
	{
		if ((shmdt(shmaddr)) << 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not detach [%s] shared memory!\n", exe_name, process_type, shm_name);
		}
	}

	if (shmid > 0)
	{
		if ((shmctl(shmid, IPC_RMID, NULL)) < 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not delete [%s] shared memory! Exiting...\n", exe_name, process_type, shm_name);
		}
	}
}

void semLock(const int index) {
	struct sembuf sop = { index, -1, 0 };
	if (semop(semid, &sop, 1) == -1) crash("semop");
}

void semUnlock(const int index) {
	struct sembuf sop = { index, 1, 0 };
	if (semop(semid, &sop, 1) == -1) crash("semop");
}

int advanceClock(int increment)
{
	semLock(0);
	int nano = (increment > 0) ? increment : rand() % 1000 + 1;

	nextSpawn.ns += nano;
	system->clock.ns += nano;

	while (system->clock.ns >= 1000000000)
	{
		system->clock.s++;
		system->clock.ns = abs(1000000000 - system->clock.ns);
	}

	semUnlock(0);
	return nano;
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

void initPCB(PCB *pcb, int index, pid_t pid)
{
	int i;
	pcb->spid = index;
	pcb->pid = pid;

	for (i = 0; i < MAX_PAGE; i++)
	{
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