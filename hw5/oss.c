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
#define system _system

static char *programName;

static int shmid = -1;
static int msqid = -1;
static int semid = -1;

static System *system = NULL;
static Message message;

static int fork_number = 0;
static pid_t pid = -1;

static Queue *queue;
static Time forkclock;
static Data data;

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
void signalHandler(int);
void timer(int);
void initIPC();
void freeIPC();
void finalize();
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

	memset(pids, 0, sizeof(pids));

	system->clock.s = 0;
	system->clock.ns = 0;
	forkclock.s = 0;
	forkclock.ns = 0;

	FILE *fp;
	if ((fp = fopen(PATH_LOG, "w")) == NULL) crash("fopen");
	if (fclose(fp) == EOF) crash("fclose");

	initPCBT(system->ptable);

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
					char exec_index[BUFFER_LENGTH];
					sprintf(exec_index, "%d", spid);
					execl("./user", "./user", exec_index, NULL);
					crash("execl");
				}

				fork_number++;
				initPCB(&system->ptable[spid], spid, pid, data);
				queue_push(queue, spid);
				log("%s: [%d.%d] p%d created\n", programName, system->clock.s, system->clock.ns, spid);
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
			message.type = system->ptable[c_index].pid;
			message.spid = c_index;
			message.pid = system->ptable[c_index].pid;
			msgsnd(msqid, &message, (sizeof(Message) - sizeof(long)), 0);
			msgrcv(msqid, &message, (sizeof(Message) - sizeof(long)), 1, 0);

			incShmclock();

			if (message.terminate == TERMINATE) {
				log("%s: [%d.%d] p%d terminating\n", programName, system->clock.s, system->clock.ns, message.spid);

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

			if (message.request) {
				log("%s: [%d.%d] p%d requesting\n", programName, system->clock.s, system->clock.ns, message.spid);

				bool isSafe = bankerAlgorithm(&data, system->ptable, queue, c_index);

				message.type = system->ptable[c_index].pid;
				message.safe = (isSafe) ? true : false;
				msgsnd(msqid, &message, (sizeof(Message) - sizeof(long)), 0);
			}

			incShmclock();

			if (message.release) {
				log("%s: [%d.%d] p%d releasing\n", programName, system->clock.s, system->clock.ns, message.spid);
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
			signalHandler(0);
		}
	}

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

void registerSignalHandlers() {
	timer(TIMEOUT);

	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &signalHandler;
	sa1.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa1, NULL) == -1) crash("sigaction");

	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &signalHandler;
	sa2.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa2, NULL) == -1) crash("sigaction");

	//Signal Handling for: SIGUSR1
	signal(SIGUSR1, SIG_IGN);
}

void signalHandler(int signum)
{
	finalize();

	fprintf(stderr, "System time: %d.%d\n", system->clock.s, system->clock.ns);
	fprintf(stderr, "Total processes executed: %d\n", fork_number);

	freeIPC();

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

void semaLock(int index) {
	struct sembuf sop = { index, -1, 0 };
	semop(semid, &sop, 1);
}

void semaRelease(const int index) {
	struct sembuf sop = { index, 1, 0 };
	semop(semid, &sop, 1);
}

void incShmclock()
{
	semaLock(0);
	int r_nano = rand() % 1000000 + 1;

	forkclock.ns += r_nano;
	system->clock.ns += r_nano;

	if (system->clock.ns >= 1000000000)
	{
		system->clock.s++;
		system->clock.ns = 1000000000 - system->clock.ns;
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
	key_t key;

	if ((key = ftok(".", 0)) == -1) crash("ftok");
	if ((shmid = shmget(key, sizeof(Time), IPC_EXCL | IPC_CREAT | 0600)) == -1) crash("shmget");
	system = (System*) shmat(shmid, NULL, 0);
	if (system == (void*) -1) crash("shmat");

	if ((key = ftok(".", 1)) == -1) crash("ftok");
	if ((msqid = msgget(key, IPC_EXCL | IPC_CREAT | 0600)) == -1) crash("msgget");

	if ((key = ftok(".", 2)) == -1) crash("ftok");
	if ((semid = semget(key, 1, IPC_EXCL | IPC_CREAT | 0600)) == -1) crash("semget");
	if (semctl(semid, 0, SETVAL, 1) == -1) crash("semctl");
}

void freeIPC() {
	if (system != NULL && shmdt(system) == -1) crash("shmdt");
	if (shmid > 0 && shmctl(shmid, IPC_RMID, NULL) == -1) crash("shmdt");

	if (msqid > 0 && msgctl(msqid, IPC_RMID, NULL) == -1) crash("msgctl");

	if (semid > 0 && semctl(semid, 0, IPC_RMID) == -1) crash("semctl");
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