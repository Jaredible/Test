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
static key_t key;
static Queue *queue;
static Time forkclock;

static int mqueueid = -1;
static Message master_message;
static int shmclock_shmid = -1;
static Time *shmclock_shmptr = NULL;
static int semid = -1;
static struct sembuf sema_operation;
static int pcbt_shmid = -1;
static PCB *pcbt_shmptr = NULL;

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
void exitHandler(int);
void timer(int);
void finalize();
void discardShm(int shmid, void *shmaddr, char *shm_name, char *exe_name, char *process_type);
void cleanUp();
void semLock(const int);
void semUnlock(const int);
int advanceClock(int);

void initPCBT(PCB*);
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

	memset(bitmap, '\0', sizeof(bitmap));

	key = ftok("./oss.c", 1);
	mqueueid = msgget(key, IPC_CREAT | 0600);
	if (mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [message queue] shared memory! Exiting...\n", programName);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	key = ftok("./oss.c", 2);
	shmclock_shmid = shmget(key, sizeof(Time), IPC_CREAT | 0600);
	if (shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [shmclock] shared memory! Exiting...\n", programName);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	shmclock_shmptr = shmat(shmclock_shmid, NULL, 0);
	if (shmclock_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", programName);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	shmclock_shmptr->s = 0;
	shmclock_shmptr->ns = 0;
	forkclock.s = 0;
	forkclock.ns = 0;

	key = ftok("./oss.c", 3);
	semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
	if (semid == -1)
	{
		fprintf(stderr, "%s ERROR: failed to create a new private semaphore! Exiting...\n", programName);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	semctl(semid, 0, SETVAL, 1);

	key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(PCB) * PROCESSES_MAX;
	pcbt_shmid = shmget(key, process_table_size, IPC_CREAT | 0600);
	if (pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [pcbt] shared memory! Exiting...\n", programName);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	pcbt_shmptr = shmat(pcbt_shmid, NULL, 0);
	if (pcbt_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", programName);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	initPCBT(pcbt_shmptr);

	queue = queue_create();
	reference_string = list_create();
	lru_stack = list_create();

	masterInterrupt(TIMEOUT);

	fprintf(stderr, "Using Least Recently Use (LRU) algorithm.\n");

	FILE *fp;
	if ((fp = fopen("output.log", "w")) == NULL) perror("ERROR");
	if (fclose(fp) == EOF) perror("ERROR");

	int last_index = -1;
	while (1)
	{
		int spawn_nano = rand() % 500000000 + 1000000;
		if (forkclock.ns >= spawn_nano)
		{
			forkclock.ns = 0;

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

			if (is_bitmap_open == true)
			{
				pid = fork();

				if (pid == -1)
				{
					fprintf(stderr, "%s (Master) ERROR: %s\n", programName, strerror(errno));
					finalize();
					cleanUp();
					exit(0);
				}

				if (pid == 0)
				{
					signal(SIGUSR1, exitHandler);

					char arg0[BUFFER_LENGTH];
					char arg1[BUFFER_LENGTH];
					sprintf(arg0, "%d", last_index);
					sprintf(arg1, "%d", scheme);
					int exect_status = execl("./user", "user", arg0, arg1, (char*) NULL);
					if (exect_status == -1)
					{
						fprintf(stderr, "%s (Child) ERROR: execl fail to execute at index [%d]! Exiting...\n", programName, last_index);
					}

					finalize();
					cleanUp();
					exit(EXIT_FAILURE);
				} else {
					fork_number++;

					bitmap[last_index / 8] |= (1 << (last_index % 8));

					initPCB(&pcbt_shmptr[last_index], last_index, pid);

					queue_push(queue, last_index);

					log("%s: generating process with PID (%d) [%d] and putting it in queue at time %d.%d\n", programName,
							   pcbt_shmptr[last_index].spid, pcbt_shmptr[last_index].pid, shmclock_shmptr->s, shmclock_shmptr->ns);
				}
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
			master_message.type = pcbt_shmptr[index].pid;
			master_message.spid = index;
			master_message.pid = pcbt_shmptr[index].pid;
			msgsnd(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 0);

			msgrcv(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 1, 0);

			advanceClock(0);

			if (master_message.flag == 0)
			{
				log("%s: process with PID (%d) [%d] has finish running at my time %d.%d\n",
						   programName, master_message.spid, master_message.pid, shmclock_shmptr->s, shmclock_shmptr->ns);

				int i;
				for (i = 0; i < MAX_PAGE; i++)
				{
					if (pcbt_shmptr[index].ptable[i].frame != -1)
					{
						int frame = pcbt_shmptr[index].ptable[i].frame;
						list_remove(reference_string, index, i, frame);
						main_memory[frame / 8] &= ~(1 << (frame % 8));
					}
				}
			}
			else
			{
				total_access_time += advanceClock(0);
				queue_push(temp, index);

				unsigned int address = master_message.address;
				unsigned int request_page = master_message.page;
				if (pcbt_shmptr[index].ptable[request_page].protection == 0)
				{
					log("%s: process (%d) [%d] requesting read of address (%d) [%d] at time %d:%d\n",
							   programName, master_message.spid, master_message.pid,
							   address, request_page,
							   shmclock_shmptr->s, shmclock_shmptr->ns);
				}
				else
				{
					log("%s: process (%d) [%d] requesting write of address (%d) [%d] at time %d:%d\n",
							   programName, master_message.spid, master_message.pid,
							   address, request_page,
							   shmclock_shmptr->s, shmclock_shmptr->ns);
				}
				memoryaccess_number++;

				if (pcbt_shmptr[index].ptable[request_page].valid == 0)
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
						pcbt_shmptr[index].ptable[request_page].frame = last_frame;
						pcbt_shmptr[index].ptable[request_page].valid = 1;

						main_memory[last_frame / 8] |= (1 << (last_frame % 8));

						list_add(reference_string, index, request_page, last_frame);
						log("%s: allocated frame [%d] to PID (%d) [%d]\n",
								   programName, last_frame, master_message.spid, master_message.pid);

						list_remove(lru_stack, index, request_page, last_frame);
						list_add(lru_stack, index, request_page, last_frame);

						if (pcbt_shmptr[index].ptable[request_page].protection == 0)
						{
							log("%s: address (%d) [%d] in frame (%d), giving data to process (%d) [%d] at time %d:%d\n",
									   programName, address, request_page,
									   pcbt_shmptr[index].ptable[request_page].frame,
									   master_message.spid, master_message.pid,
									   shmclock_shmptr->s, shmclock_shmptr->ns);

							pcbt_shmptr[index].ptable[request_page].dirty = 0;
						}
						else
						{
							log("%s: address (%d) [%d] in frame (%d), writing data to frame at time %d:%d\n",
									   programName, address, request_page,
									   pcbt_shmptr[index].ptable[request_page].frame,
									   shmclock_shmptr->s, shmclock_shmptr->ns);

							pcbt_shmptr[index].ptable[request_page].dirty = 1;
						}
					} else {
						log("%s: address (%d) [%d] is not in a frame, memory is full. Invoking page replacement...\n",
								   programName, address, request_page);

						unsigned int lru_index = lru_stack->head->index;
						unsigned int lru_page = lru_stack->head->page;
						unsigned int lru_address = lru_page << 10;
						unsigned int lru_frame = lru_stack->head->frame;

						if (pcbt_shmptr[lru_index].ptable[lru_page].dirty == 1) {
							log("%s: address (%d) [%d] was modified. Modified information is written back to the disk\n",
									   programName, lru_address, lru_page);
						}

						pcbt_shmptr[lru_index].ptable[lru_page].frame = -1;
						pcbt_shmptr[lru_index].ptable[lru_page].dirty = 0;
						pcbt_shmptr[lru_index].ptable[lru_page].valid = 0;

						pcbt_shmptr[index].ptable[request_page].frame = lru_frame;
						pcbt_shmptr[index].ptable[request_page].dirty = 0;
						pcbt_shmptr[index].ptable[request_page].valid = 1;

						list_remove(lru_stack, lru_index, lru_page, lru_frame);
						list_remove(reference_string, lru_index, lru_page, lru_frame);
						list_add(lru_stack, index, request_page, lru_frame);
						list_add(reference_string, index, request_page, lru_frame);

						if (pcbt_shmptr[index].ptable[request_page].protection == 1)
						{
							log("%s: dirty bit of frame (%d) set, adding additional time to the clock\n", programName, last_frame);
							log("%s: indicating to process (%d) [%d] that write has happend to address (%d) [%d]\n",
									   programName, master_message.spid, master_message.pid, address, request_page);
							pcbt_shmptr[index].ptable[request_page].dirty = 1;
						}
					}
				} else {
					int frame = pcbt_shmptr[index].ptable[request_page].frame;
					list_remove(lru_stack, index, request_page, frame);
					list_add(lru_stack, index, request_page, frame);

					if (pcbt_shmptr[index].ptable[request_page].protection == 0) {
						log("%s: address (%d) [%d] is already in frame (%d), giving data to process (%d) [%d] at time %d:%d\n",
								   programName, address, request_page,
								   pcbt_shmptr[index].ptable[request_page].frame,
								   master_message.spid, master_message.pid,
								   shmclock_shmptr->s, shmclock_shmptr->ns);
					} else {
						log("%s: address (%d) [%d] is already in frame (%d), writing data to frame at time %d:%d\n",
								   programName, address, request_page,
								   pcbt_shmptr[index].ptable[request_page].frame,
								   shmclock_shmptr->s, shmclock_shmptr->ns);
					}
				}
			}

			next = (next->next != NULL) ? next->next : NULL;

			master_message.type = -1;
			master_message.spid = -1;
			master_message.pid = -1;
			master_message.flag = -1;
			master_message.page = -1;
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

	double mem_p_sec = (double)memoryaccess_number / (double)shmclock_shmptr->s;
	double pg_f_p_mem = (double)pagefault_number / (double)memoryaccess_number;
	double avg_m = (double)total_access_time / (double)memoryaccess_number;
	avg_m /= 1000000.0;

	log("- Master PID: %d\n", getpid());
	log("- Number of forking during this execution: %d\n", fork_number);
	log("- Final simulation time of this execution: %d.%d\n", shmclock_shmptr->s, shmclock_shmptr->ns);
	log("- Number of memory accesses: %d\n", memoryaccess_number);
	log("- Number of memory accesses per nanosecond: %f memory/second\n", mem_p_sec);
	log("- Number of page faults: %d\n", pagefault_number);
	log("- Number of page faults per memory access: %f pagefault/access\n", pg_f_p_mem);
	log("- Average memory access speed: %f ms/n\n", avg_m);
	log("- Total memory access time: %f ms\n", (double)total_access_time / 1000000.0);
	fprintf(stderr, "SIMULATION RESULT is recorded into the log file: %s\n", "output.log");

	cleanUp();

	exit(EXIT_SUCCESS);
}

void segHandler(int signum) {
	fprintf(stderr, "Segmentation Fault\n");
	masterHandler(0);
}

void exitHandler(int signum) {
	fprintf(stderr, "%d: Terminated!\n", getpid());
	exit(EXIT_SUCCESS);
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
	fprintf(stderr, "\nLimitation has reached! Invoking termination...\n");
	kill(0, SIGUSR1);
	pid_t p = 0;
	while (p >= 0)
		p = waitpid(-1, NULL, WNOHANG);
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

void cleanUp()
{
	if (mqueueid > 0)
	{
		msgctl(mqueueid, IPC_RMID, NULL);
	}

	discardShm(shmclock_shmid, shmclock_shmptr, "shmclock", programName, "Master");

	if (semid > 0)
	{
		semctl(semid, 0, IPC_RMID);
	}

	discardShm(pcbt_shmid, pcbt_shmptr, "pcbt", programName, "Master");
}

void semLock(const int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = -1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

void semUnlock(const int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = 1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

int advanceClock(int increment)
{
	semLock(0);
	int nano = (increment > 0) ? increment : rand() % 1000 + 1;

	forkclock.ns += nano;
	shmclock_shmptr->ns += nano;

	while (shmclock_shmptr->ns >= 1000000000)
	{
		shmclock_shmptr->s++;
		shmclock_shmptr->ns = abs(1000000000 - shmclock_shmptr->ns);
	}

	semUnlock(0);
	return nano;
}

void initPCBT(PCB *pcbt)
{
	int i, j;
	for (i = 0; i < PROCESSES_MAX; i++)
	{
		pcbt[i].spid = -1;
		pcbt[i].pid = -1;
		for (j = 0; j < MAX_PAGE; j++)
		{
			pcbt[i].ptable[j].frame = -1;
			pcbt[i].ptable[j].protection = rand() % 2;
			pcbt[i].ptable[j].dirty = 0;
			pcbt[i].ptable[j].valid = 0;
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