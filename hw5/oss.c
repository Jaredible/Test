/*
 * oss.c November 21, 2020
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
#include "queue.h"

#define log _log

/* Static GLOBAL variable (misc) */
static char *programName;
static key_t key;
static Queue *queue;
static Time forkclock;
static Data data;
/* -------------------------------------------------- */

/* Static GLOBAL variable (shared memory) */
static int mqueueid = -1;
static Message master_message;
static int shmclock_shmid = -1;
static Time *shmclock_shmptr = NULL;
static int semid = -1;
static struct sembuf sema_operation;
static int pcbt_shmid = -1;
static PCB *pcbt_shmptr = NULL;
/* -------------------------------------------------- */

/* Static GLOBAL variable (fork) */
static int fork_number = 0;
static pid_t pid = -1;
/* -------------------------------------------------- */

/* Prototype Function */
void masterHandler(int);
void exitHandler(int);
void finalize();
void discardShm(int, void*, char*, char*, char*);
void cleanUp();
void semaLock(int);
void semaRelease(int);
void incShmclock();

void initResource(Data*);
void displayResource(Data);
void updateResource(Data*, PCB*);

void initPCBT(PCB*);
void initPCB(PCB*, int, pid_t, Data);

void setMatrix(PCB*, Queue*, int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int);
void calculateNeedMatrix(Data*, int need[][RESOURCES_MAX], int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int);
void displayVector(char*, char*, int vector[RESOURCES_MAX]);
void displayMatrix(char*, Queue*, int matrix[][RESOURCES_MAX], int);
bool bankerAlgorithm(Data*, PCB*, Queue*, int);

void init(int, char**);
void error(char*, ...);
void crash(char*);
void usage(int);
void registerSignalHandlers();
void timer(int);
void initIPC();
void freeIPC();
void cleanup();
void log(char*, ...);
int findAvailablePID();

bool verbose = false;
pid_t pids[PROCESSES_MAX];

int main(int argc, char **argv) {
	init(argc, argv);

	srand(time(NULL) ^ getpid());

	bool ok = true;

	while (true) {
		int c = getopt(argc, argv, "hv");
		if (c == -1) break;
		switch (c) {
			case 'h':
				usage(EXIT_SUCCESS);
			case 'v':
				verbose = true;
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

	shmclock_shmptr->s = 0;
	shmclock_shmptr->ns = 0;
	forkclock.s = 0;
	forkclock.ns = 0;

	FILE *fp;
	if ((fp = fopen(PATH_LOG, "w")) == NULL) crash("fopen");
	if (fclose(fp) == EOF) crash("fclose");

	initPCBT(pcbt_shmptr);

	queue = queue_create();
	initResource(&data);
	displayResource(data);

	registerSignalHandlers();

	while (true) {
		//int spawn_nano = rand() % 500000000 + 1000000;
		int spawn_nano = 100;
		if (forkclock.ns >= spawn_nano) {
			//Reset forkclock
			forkclock.ns = 0;

			int spid = findAvailablePID();
			if (spid >= 0) {
				pid = fork();
				pids[spid] = pid;

				if (pid == -1) crash("fork");
				else if (pid == 0) {
					signal(SIGUSR1, exitHandler);
					char exec_index[BUFFER_LENGTH];
					sprintf(exec_index, "%d", spid);
					execl("./user", "./user", exec_index, NULL);
					crash("execl");
				}

				fork_number++;
				initPCB(&pcbt_shmptr[spid], spid, pid, data);
				queue_push(queue, spid);
				log("%s: [%d.%d] p%d created\n", programName, shmclock_shmptr->s, shmclock_shmptr->ns, spid);
			}
		}

		incShmclock();

		QueueNode next;
		Queue *trackingQueue = queue_create();

		int current_iteration = 0;
		next.next = queue->front;
		while (next.next != NULL) {
			incShmclock();

			int c_index = next.next->index;
			master_message.type = pcbt_shmptr[c_index].pid;
			master_message.spid = c_index;
			master_message.pid = pcbt_shmptr[c_index].pid;
			msgsnd(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 0);
			msgrcv(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 1, 0);

			incShmclock();

			if (master_message.action == TERMINATE) {
				log("%s: [%d.%d] p%d terminating\n", programName, shmclock_shmptr->s, shmclock_shmptr->ns, master_message.spid);

				QueueNode current;
				current.next = queue->front;
				while (current.next != NULL) {
					if (current.next->index != c_index) {
						queue_push(trackingQueue, current.next->index);
					}

					current.next = (current.next->next != NULL) ? current.next->next : NULL;
				}

				while (!queue_empty(queue)) {
					queue_pop(queue);
				}
				while (!queue_empty(trackingQueue)) {
					int i = trackingQueue->front->index;
					queue_push(queue, i);
					queue_pop(trackingQueue);
				}

				next.next = queue->front;
				int i;
				for (i = 0; i < current_iteration; i++) {
					next.next = (next.next->next != NULL) ? next.next->next : NULL;
				}
				continue;
			}

			if (master_message.action == REQUEST) {
				log("%s: [%d.%d] p%d requesting\n", programName, shmclock_shmptr->s, shmclock_shmptr->ns, master_message.spid);

				bool isSafe = bankerAlgorithm(&data, pcbt_shmptr, queue, c_index);

				master_message.type = pcbt_shmptr[c_index].pid;
				master_message.safe = (isSafe) ? true : false;
				msgsnd(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 0);
			}

			incShmclock();

			if (master_message.action == RELEASE) {
				log("%s: [%d.%d] p%d releasing\n", programName, shmclock_shmptr->s, shmclock_shmptr->ns, master_message.spid);
			}

			current_iteration++;

			next.next = (next.next->next != NULL) ? next.next->next : NULL;
		}

		free(trackingQueue);

		incShmclock();

		int child_status = 0;
		pid_t child_pid = waitpid(-1, &child_status, WNOHANG);

		if (child_pid > 0) {
			int return_index = WEXITSTATUS(child_status);
			pids[return_index] = 0;
		}

		if (fork_number >= PROCESSES_TOTAL) {
			timer(0);
			masterHandler(0);
		}
	}

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

void registerSignalHandlers() {
	timer(TIMEOUT);

	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &masterHandler;
	sa1.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa1, NULL) == -1) crash("sigaction");

	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &masterHandler;
	sa2.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa2, NULL) == -1) crash("sigaction");

	//Signal Handling for: SIGUSR1
	signal(SIGUSR1, SIG_IGN);
}

void masterHandler(int signum)
{
	finalize();

	//Print out basic statistic
	fprintf(stderr, "- Master PID: %d\n", getpid());
	fprintf(stderr, "- Number of forking during this execution: %d\n", fork_number);
	fprintf(stderr, "- Final simulation time of this execution: %d.%d\n", shmclock_shmptr->s, shmclock_shmptr->ns);

	cleanUp();

	exit(EXIT_SUCCESS);
}

void exitHandler(int signum)
{
//	printf("%d: Terminated!\n", getpid());
	exit(EXIT_SUCCESS);
}

void finalize()
{
	fprintf(stderr, "\nLimitation has reached! Invoking termination...\n");
	kill(0, SIGUSR1);
	pid_t p = 0;
	while (p >= 0)
	{
		p = waitpid(-1, NULL, WNOHANG);
	}
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
	//Delete [message queue] shared memory
	if (mqueueid > 0)
	{
		msgctl(mqueueid, IPC_RMID, NULL);
	}

	//Release and delete [shmclock] shared memory
	discardShm(shmclock_shmid, shmclock_shmptr, "shmclock", programName, "Master");

	//Delete semaphore
	if (semid > 0)
	{
		semctl(semid, 0, IPC_RMID);
	}

	//Release and delete [pcbt] shared memory
	discardShm(pcbt_shmid, pcbt_shmptr, "pcbt", programName, "Master");
}

void semaLock(int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = -1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

void semaRelease(int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = 1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}

void incShmclock()
{
	semaLock(0);
	int r_nano = rand() % 1000000 + 1;

	forkclock.ns += r_nano;
	shmclock_shmptr->ns += r_nano;

	if (shmclock_shmptr->ns >= 1000000000)
	{
		shmclock_shmptr->s++;
		shmclock_shmptr->ns = 1000000000 - shmclock_shmptr->ns;
	}

	semaRelease(0);
}

void initResource(Data *data)
{
	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		data->init[i] = rand() % 10 + 1;
		data->resource[i] = data->init[i];
	}

	data->shared = (SHARED_RESOURCES_MAX == 0) ? 0 : rand() % (SHARED_RESOURCES_MAX - (SHARED_RESOURCES_MAX - SHARED_RESOURCES_MIN)) + SHARED_RESOURCES_MIN;
}

void displayResource(Data data)
{
	log("===Total Resource===\n<");
	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		log("%2d", data.resource[i]);

		if (i < RESOURCES_MAX - 1)
		{
			log(" | ");
		}
	}
	log(">\n\n");

	log("Sharable Resources: %d\n", data.shared);
}

void initPCBT(PCB *pcbt)
{
	int i;
	for (i = 0; i < PROCESSES_MAX; i++)
	{
		pcbt[i].spid = -1;
		pcbt[i].pid = -1;
	}
}

void initPCB(PCB *pcb, int index, pid_t pid, Data data)
{
	pcb->spid = index;
	pcb->pid = pid;

	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		pcb->maximum[i] = rand() % (data.init[i] + 1);
		pcb->allocation[i] = 0;
		pcb->request[i] = 0;
		pcb->release[i] = 0;
	}
}

void setMatrix(PCB *pcbt, Queue *queue, int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int count)
{
	QueueNode next;
	next.next = queue->front;

	int i, j;
	int c_index = next.next->index;
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			maxm[i][j] = pcbt[c_index].maximum[j];
			allot[i][j] = pcbt[c_index].allocation[j];
		}

		if (next.next->next != NULL)
		{
			next.next = next.next->next;
			c_index = next.next->index;
		}
		else
		{
			next.next = NULL;
		}
	}
}

void calculateNeedMatrix(Data *data, int need[][RESOURCES_MAX], int maxm[][RESOURCES_MAX], int allot[][RESOURCES_MAX], int count)
{
	int i, j;
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			need[i][j] = maxm[i][j] - allot[i][j];
		}
	}
}

void displayVector(char *v_name, char *l_name, int vector[RESOURCES_MAX])
{
	log("===%s Resource===\n%3s :  <", v_name, l_name);

	int i;
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		log("%2d", vector[i]);

		if (i < RESOURCES_MAX - 1)
		{
			log(" | ");
		}
	}
	log(">\n");
}

void displayMatrix(char *m_name, Queue *queue, int matrix[][RESOURCES_MAX], int count)
{
	QueueNode next;
	next.next = queue->front;

	int i, j;
	log("===%s Matrix===\n", m_name);

	for (i = 0; i < count; i++)
	{
		log("P%2d :  <", next.next->index);
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			log("%2d", matrix[i][j]);

			if (j < RESOURCES_MAX - 1)
			{
				log(" | ");
			}
		}
		log(">\n");

		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}
}

bool bankerAlgorithm(Data *data, PCB *pcbt, Queue *queue, int c_index) {
	int i, p, j, k;

	//=====Check for null queue=====
	//Return true (safe) when working queue is null
	QueueNode next;
	next.next = queue->front;
	if (next.next == NULL)
	{
		return true;
	}

	//=====Initialization/Get information=====
	int count = queue_size(queue);
	int maxm[count][RESOURCES_MAX];
	int allot[count][RESOURCES_MAX];
	int req[RESOURCES_MAX];
	int need[count][RESOURCES_MAX];
	int avail[RESOURCES_MAX];

	//Setting up matrix base on given queue and process control block table
	setMatrix(pcbt, queue, maxm, allot, count);

	//Calculate need matrix
	calculateNeedMatrix(data, need, maxm, allot, count);

	//Setting up available vector and request vector
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		avail[i] = data->resource[i];
		req[i] = pcbt[c_index].request[i];
	}

	//Update available vector
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < RESOURCES_MAX; j++)
		{
			avail[j] = avail[j] - allot[i][j];
		}
	}

	//Map the PID Index to NEED vector index
	int idx = 0;
	next.next = queue->front;
	while (next.next != NULL)
	{
		if (next.next->index == c_index)
		{
			break;
		}
		idx++;

		//Point the pointer to the next queue element
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}

	//Display information
	if (verbose)
	{
		displayMatrix("Maximum", queue, maxm, count);
		displayMatrix("Allocation", queue, allot, count);
		char str[BUFFER_LENGTH];
		sprintf(str, "P%2d", c_index);
		displayVector("Request", str, req);
	}

	//=====Finding SAFE Sequence=====
	bool finish[count];							  //To store finish
	int safeSeq[count];							  //To store safe sequence
	memset(finish, 0, count * sizeof(finish[0])); //Mark all processes as not finish

	//Make a copy of available resources (working vector)
	int work[RESOURCES_MAX];
	for (i = 0; i < RESOURCES_MAX; i++)
	{
		work[i] = avail[i];
	}

	/* =====Resource Request Algorithm===== */
	for (j = 0; j < RESOURCES_MAX; j++)
	{
		//Check to see if the process is not asking for more than it will ever need
		if (need[idx][j] < req[j] && j < data->shared)
		{
			log("\tAsked for more than initial max request\n");

			//Display information
			if (verbose)
			{
				displayVector("Available", "A  ", avail);
				displayMatrix("Need", queue, need, count);
			}
			return false;
		}

		if (req[j] <= avail[j] && j < data->shared)
		{
			avail[j] -= req[j];
			allot[idx][j] += req[j];
			need[idx][j] -= req[j];
		}
		else
		{
			log("\tNot enough available resources\n");

			//Display information
			if (verbose)
			{
				displayVector("Available", "A  ", avail);
				displayMatrix("Need", queue, need, count);
			}
			return false;
		}
	}

	/* =====Safety Algorithm===== */
	//While all processes are not finished or system is not in safe state.
	int index = 0;
	while (index < count)
	{
		//Find a process which is not finish and whose needs can be satisfied with current work[] resources.
		bool found = false;
		for (p = 0; p < count; p++)
		{
			//First check if a process is finished, if no, go for next condition
			if (finish[p] == 0)
			{
				//Check if for all resources of current process need is less than work
				for (j = 0; j < RESOURCES_MAX; j++)
				{
					if (need[p][j] > work[j] && data->shared)
					{
						break;
					}
				}

				//If all needs of p were satisfied.
				if (j == RESOURCES_MAX)
				{
					//Add the allocated resources of current process to the available/work resources i.e. free the resources
					for (k = 0; k < RESOURCES_MAX; k++)
					{
						work[k] += allot[p][k];
					}

					//Add this process to safe sequence.
					safeSeq[index++] = p;

					//Mark this p as finished and found is true
					finish[p] = 1;
					found = true;
				}
			}
		}

		//If we could not find a next process in safe sequence.
		if (found == false)
		{
			log("System is in UNSAFE (not safe) state\n");
			return false;
		}
	} //END OF: index < count

	//Display information
	if (verbose)
	{
		displayVector("Available", "A  ", avail);
		displayMatrix("Need", queue, need, count);
	}

	//Map the safe sequence with the queue sequence
	int sequence[count];
	int seq_index = 0;
	next.next = queue->front;
	while (next.next != NULL)
	{
		sequence[seq_index++] = next.next->index;

		//Point the pointer to the next queue element
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}

	//If system is in safe state then safe sequence will be as below
	log("System is in SAFE state. Safe sequence is: ");
	for (i = 0; i < count; i++)
	{
		log("%2d ", sequence[safeSeq[i]]);
	}
	log("\n\n");

	return true;
}

void initIPC() {
	memset(pids, 0, sizeof(pids));

	//--------------------------------------------------
	/* =====Initialize message queue===== */
	//Allocate shared memory if doesn't exist, and check if it can create one. Return ID for [message queue] shared memory
	key = ftok("./oss.c", 1);
	mqueueid = msgget(key, IPC_CREAT | 0600);
	if(mqueueid < 0) crash("msgget");


	//--------------------------------------------------
	/* =====Initialize [shmclock] shared memory===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [shmclock] shared memory
	key = ftok("./oss.c", 2);
	shmclock_shmid = shmget(key, sizeof(Time), IPC_CREAT | 0600);
	if(shmclock_shmid < 0) crash("shmget");

	//Attaching shared memory and check if can attach it. If not, delete the [shmclock] shared memory
	shmclock_shmptr = shmat(shmclock_shmid, NULL, 0);
	if(shmclock_shmptr == (void *)( -1 )) crash("shmat");

	//Initialize shared memory attribute of [shmclock] and forkclock
	shmclock_shmptr->s = 0;
	shmclock_shmptr->ns = 0;
	forkclock.s = 0;
	forkclock.ns = 0;

	//--------------------------------------------------
	/* =====Initialize semaphore===== */
	//Creating 3 semaphores elements
	//Create semaphore if doesn't exist with 666 bits permission. Return error if semaphore already exists
	key = ftok("./oss.c", 3);
	semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
	if(semid == -1) crash("semget");
	
	//Initialize the semaphore(s) in our set to 1
	semctl(semid, 0, SETVAL, 1);	//Semaphore #0: for [shmclock] shared memory
	

	//--------------------------------------------------
	/* =====Initialize process control block table===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [pcbt] shared memory
	key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(PCB) * PROCESSES_MAX;
	pcbt_shmid = shmget(key, process_table_size, IPC_CREAT | 0600);
	if(pcbt_shmid < 0) crash("shmget");

	//Attaching shared memory and check if can attach it. If not, delete the [pcbt] shared memory
	pcbt_shmptr = shmat(pcbt_shmid, NULL, 0);
	if(pcbt_shmptr == (void *)( -1 )) crash("shmat");
}

void freeIPC() {
}

void crash(char *msg) {
	char buf[BUFFER_LENGTH];
	snprintf(buf, BUFFER_LENGTH, "%s: %s", programName, msg);
	perror(buf);
	
	cleanup();
	
	exit(EXIT_FAILURE);
}

void error(char *fmt, ...) {
	char buf[BUFFER_LENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, BUFFER_LENGTH, fmt, args);
	va_end(args);
	
	fprintf(stderr, "%s: %s\n", programName, buf);
	
	cleanup();
}

void usage(int status) {
	if (status != EXIT_SUCCESS) fprintf(stderr, "Try '%s -h' for more information\n", programName);
	else {
		printf("Usage: %s [-v]\n", programName);
		printf("    -v : verbose on\n");
	}
	exit(status);
}

void init(int argc, char **argv) {
	programName = argv[0];

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void cleanup() {
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

int findAvailablePID() {
	int i;
	for (i = 0; i < PROCESSES_MAX; i++)
		if (pids[i] == 0) return i;
	return -1;
}

void timer(int duration) {
	struct itimerval val;
	val.it_value.tv_sec = duration;
	val.it_value.tv_usec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &val, NULL) == -1) crash("setitimer");
}