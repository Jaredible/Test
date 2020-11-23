

#include <stdlib.h>	 //exit()
#include <stdio.h>	 //printf()
#include <stdbool.h> //bool variable
#include <stdint.h>	 //for uint32_t
#include <string.h>	 //str function
#include <unistd.h>	 //standard symbolic constants and types

#include <stdarg.h>	   //va macro
#include <errno.h>	   //errno variable
#include <signal.h>	   //signal handling
#include <sys/ipc.h>   //IPC flags
#include <sys/msg.h>   //message queue stuff
#include <sys/shm.h>   //shared memory stuff
#include <sys/sem.h>   //semaphore stuff, semget()
#include <sys/time.h>  //setitimer()
#include <sys/types.h> //contains a number of basic derived types
#include <sys/wait.h>  //waitpid()
#include <time.h>	   //time()
#include <math.h>	   //ceil(), floor()

#include "shared.h"
#include "queue.h"
#include "list.h"

#define log _log

static char *programName;

static FILE *fpw = NULL;
static char *exe_name;
static int percentage = 0;
static char log_file[256] = "fifolog.dat";
static char isDebugMode = false;
static char isDisplayTerminal = false;
static int scheme_choice = 0;
static int algorithm_choice = 1;
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
static unsigned char bitmap[MAX_PROCESS];

static int memoryaccess_number = 0;
static int pagefault_number = 0;
static unsigned int total_access_time = 0;
static unsigned char main_memory[MAX_FRAME];
static int last_frame = -1;
static List *reference_string;
static List *lru_stack;

void log(FILE *fpw, char *fmt, ...);
void masterInterrupt(int seconds);
void masterHandler(int signum);
void segHandler(int signum);
void exitHandler(int signum);
void timer(int seconds);
void finalize();
void discardShm(int shmid, void *shmaddr, char *shm_name, char *exe_name, char *process_type);
void cleanUp();
void semaLock(int sem_index);
void semaRelease(int sem_index);
int incShmclock(int increment);

void initPCBT(PCB *pcbt);
void initPCB(PCB *pcb, int index, pid_t pid);

int main(int argc, char *argv[])
{
	exe_name = argv[0];
	srand(getpid());

	int opt;
	while ((opt = getopt(argc, argv, "hl:dta:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			printf("NAME:\n");
			printf("	%s - simulate the memory management module and compare LRU and FIFO page replacement algorithms, both with dirty-bit optimization.\n", exe_name);
			printf("\nUSAGE:\n");
			printf("	%s [-h] [-l logfile] [-a choice].\n", exe_name);
			printf("\nDESCRIPTION:\n");
			printf("	-h           : print the help page and exit.\n");
			printf("	-l filename  : the log file used (default is fifolog.dat).\n");
			printf("	-d           : turn on debug mode (default is off). Beware, result will be different and inconsistent.\n");
			printf("	-t           : display result in the terminal as well (default is off).\n");
			printf("	-a number    : 0 is using FIFO algorithm, while 1 is using LRU algorithm (default is 0).\n");
			exit(EXIT_SUCCESS);

		case 'l':
			strncpy(log_file, optarg, 255);
			fprintf(stderr, "Your new log file is: %s\n", log_file);
			break;
		case 't':
			isDisplayTerminal = true;
			break;

		case 'm':
			scheme_choice = atoi(optarg);
			scheme_choice = (scheme_choice < 0 || scheme_choice > 1) ? 0 : scheme_choice;
			break;

		default:
			fprintf(stderr, "%s: please use \"-h\" option for more info.\n", exe_name);
			exit(EXIT_FAILURE);
		}
	}

	if (algorithm_choice == 1)
	{
		strncpy(log_file, "lrulog.dat", 255);
	}

	//Check for extra arguments
	if (optind < argc)
	{
		fprintf(stderr, "%s ERROR: extra arguments was given! Please use \"-h\" option for more info.\n", exe_name);
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Initialize LOG File===== */
	fpw = fopen(log_file, "w");
	if (fpw == NULL)
	{
		fprintf(stderr, "%s ERROR: unable to write the output file.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Initialize BITMAP===== */
	//Zero out all elements of bit map
	memset(bitmap, '\0', sizeof(bitmap));

	//--------------------------------------------------
	/* =====Initialize message queue===== */
	//Allocate shared memory if doesn't exist, and check if it can create one. Return ID for [message queue] shared memory
	key = ftok("./oss.c", 1);
	mqueueid = msgget(key, IPC_CREAT | 0600);
	if (mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [message queue] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Initialize [shmclock] shared memory===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [shmclock] shared memory
	key = ftok("./oss.c", 2);
	shmclock_shmid = shmget(key, sizeof(Time), IPC_CREAT | 0600);
	if (shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [shmclock] shared memory
	shmclock_shmptr = shmat(shmclock_shmid, NULL, 0);
	if (shmclock_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

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
	if (semid == -1)
	{
		fprintf(stderr, "%s ERROR: failed to create a new private semaphore! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Initialize the semaphore(s) in our set to 1
	semctl(semid, 0, SETVAL, 1); //Semaphore #0: for [shmclock] shared memory

	//--------------------------------------------------
	/* =====Initialize process control block table===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [pcbt] shared memory
	key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(PCB) * MAX_PROCESS;
	pcbt_shmid = shmget(key, process_table_size, IPC_CREAT | 0600);
	if (pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [pcbt] shared memory
	pcbt_shmptr = shmat(pcbt_shmid, NULL, 0);
	if (pcbt_shmptr == (void *)(-1))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Init process control block table variable
	initPCBT(pcbt_shmptr);

	//--------------------------------------------------
	/* ===== Queue/Resource ===== */
	//Set up queue
	queue = queue_create();
	reference_string = list_create();
	lru_stack = list_create();

	//--------------------------------------------------
	/* =====Signal Handling===== */
	masterInterrupt(TERMINATION_TIME);

	if (isDebugMode)
	{
		fprintf(stderr, "DEBUG mode is ON\n");
	}
	if (algorithm_choice == 0)
	{
		fprintf(stderr, "Using First In First Out (FIFO) algorithm.\n");
	}
	else
	{
		fprintf(stderr, "Using Least Recently Use (LRU) algorithm.\n");
	}

	int last_index = -1;
	while (1)
	{
		int spawn_nano = (isDebugMode) ? 100 : rand() % 500000000 + 1000000;
		if (forkclock.ns >= spawn_nano)
		{
			//Reset forkclock
			forkclock.ns = 0;

			//Do bitmap has an open spot?
			bool is_bitmap_open = false;
			int count_process = 0;
			while (1)
			{
				last_index = (last_index + 1) % MAX_PROCESS;
				uint32_t bit = bitmap[last_index / 8] & (1 << (last_index % 8));
				if (bit == 0)
				{
					is_bitmap_open = true;
					break;
				}

				if (count_process >= MAX_PROCESS - 1)
				{
					break;
				}
				count_process++;
			}

			//Continue to fork if there are still space in the bitmap
			if (is_bitmap_open == true)
			{
				pid = fork();

				if (pid == -1)
				{
					fprintf(stderr, "%s (Master) ERROR: %s\n", exe_name, strerror(errno));
					finalize();
					cleanUp();
					exit(0);
				}

				if (pid == 0) //Child
				{
					//Simple signal handler: this is for mis-synchronization when timer fire off
					signal(SIGUSR1, exitHandler);

					//Replaces the current running process with a new process (user)
					char arg0[BUFFER_LENGTH];
					char arg1[BUFFER_LENGTH];
					sprintf(arg0, "%d", last_index);
					sprintf(arg1, "%d", scheme_choice);
					int exect_status = execl("./user", "user", arg0, arg1, (char*) NULL);
					if (exect_status == -1)
					{
						fprintf(stderr, "%s (Child) ERROR: execl fail to execute at index [%d]! Exiting...\n", exe_name, last_index);
					}

					finalize();
					cleanUp();
					exit(EXIT_FAILURE);
				}
				else //Parent
				{
					//Increment the total number of fork in execution
					if (!isDisplayTerminal && (fork_number == percentage))
					{
						if (fork_number < TOTAL_PROCESS - 1 || TOTAL_PROCESS % 2 != 1)
						{
							fprintf(stderr, "%c%c%c%c%c", 219, 219, 219, 219, 219);
						}
						percentage += (int)(ceil(TOTAL_PROCESS * 0.1));
					}
					fork_number++;

					//Set the current index to one bit (meaning it is taken)
					bitmap[last_index / 8] |= (1 << (last_index % 8));

					//Initialize user process information to the process control block table
					initPCB(&pcbt_shmptr[last_index], last_index, pid);

					//Add the process to highest queue
					queue_push(queue, last_index);

					//Display creation time
					log(fpw, "%s: generating process with PID (%d) [%d] and putting it in queue at time %d.%d\n", exe_name,
							   pcbt_shmptr[last_index].spid, pcbt_shmptr[last_index].pid, shmclock_shmptr->s, shmclock_shmptr->ns);
				}
			} //END OF: is_bitmap_open if check
		}	  //END OF: forkclock.nanosecond if check

		//- CRITICAL SECTION -//
		incShmclock(0);

		//--------------------------------------------------
		/* =====Main Driver Procedure===== */
		//Application procedure queues
		QueueNode qnext;
		Queue *trackingQueue = queue_create();

		qnext.next = queue->front;
		while (qnext.next != NULL)
		{
			//- CRITICAL SECTION -//
			incShmclock(0);

			//Sending a message to a specific child to tell him it is his turn
			int c_index = qnext.next->index;
			master_message.type = pcbt_shmptr[c_index].pid;
			master_message.spid = c_index;
			master_message.pid = pcbt_shmptr[c_index].pid;
			msgsnd(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 0);

			//Waiting for the specific child to respond back
			msgrcv(mqueueid, &master_message, (sizeof(Message) - sizeof(long)), 1, 0);

			//- CRITICAL SECTION -//
			incShmclock(0);

			//If child want to terminate, skips the current iteration of the loop and continues with the next iteration
			if (master_message.flag == 0)
			{
				log(fpw, "%s: process with PID (%d) [%d] has finish running at my time %d.%d\n",
						   exe_name, master_message.spid, master_message.pid, shmclock_shmptr->s, shmclock_shmptr->ns);

				//Return all allocated frame from this process
				int i;
				for (i = 0; i < MAX_PAGE; i++)
				{
					if (pcbt_shmptr[c_index].ptable[i].frame != -1)
					{
						int frame = pcbt_shmptr[c_index].ptable[i].frame;
						list_remove(reference_string, c_index, i, frame);
						main_memory[frame / 8] &= ~(1 << (frame % 8));
					}
				}
			}
			else
			{
				//- CRITICAL SECTION -//
				total_access_time += incShmclock(0);
				queue_push(trackingQueue, c_index);

				//- Allocate Frames Procedure -//
				unsigned int address = master_message.address;
				unsigned int request_page = master_message.page;
				if (pcbt_shmptr[c_index].ptable[request_page].protection == 0)
				{
					log(fpw, "%s: process (%d) [%d] requesting read of address (%d) [%d] at time %d:%d\n",
							   exe_name, master_message.spid, master_message.pid,
							   address, request_page,
							   shmclock_shmptr->s, shmclock_shmptr->ns);
				}
				else
				{
					log(fpw, "%s: process (%d) [%d] requesting write of address (%d) [%d] at time %d:%d\n",
							   exe_name, master_message.spid, master_message.pid,
							   address, request_page,
							   shmclock_shmptr->s, shmclock_shmptr->ns);
				}
				memoryaccess_number++;

				//Check for valid bit for the current page
				if (pcbt_shmptr[c_index].ptable[request_page].valid == 0)
				{
					log(fpw, "%s: address (%d) [%d] is not in a frame, PAGEFAULT\n",
							   exe_name, address, request_page);
					pagefault_number++;

					//- CRITICAL SECTION -//
					total_access_time += incShmclock(14000000);

					//Do main memory has an open spot?
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

					//Continue if there are still space in the main memory
					if (is_memory_open == true)
					{
						//Allocate frame to this page and change the valid bit
						pcbt_shmptr[c_index].ptable[request_page].frame = last_frame;
						pcbt_shmptr[c_index].ptable[request_page].valid = 1;

						//Set the current frame to one (meaning it is taken)
						main_memory[last_frame / 8] |= (1 << (last_frame % 8));

						//Frame allocated...
						list_add(reference_string, c_index, request_page, last_frame);
						log(fpw, "%s: allocated frame [%d] to PID (%d) [%d]\n",
								   exe_name, last_frame, master_message.spid, master_message.pid);

						//Update LRU stack
						if (algorithm_choice == 1)
						{
							list_remove(lru_stack, c_index, request_page, last_frame);
							list_add(lru_stack, c_index, request_page, last_frame);
						}

						//Giving data to process OR writing data to frame
						if (pcbt_shmptr[c_index].ptable[request_page].protection == 0)
						{
							log(fpw, "%s: address (%d) [%d] in frame (%d), giving data to process (%d) [%d] at time %d:%d\n",
									   exe_name, address, request_page,
									   pcbt_shmptr[c_index].ptable[request_page].frame,
									   master_message.spid, master_message.pid,
									   shmclock_shmptr->s, shmclock_shmptr->ns);

							pcbt_shmptr[c_index].ptable[request_page].dirty = 0;
						}
						else
						{
							log(fpw, "%s: address (%d) [%d] in frame (%d), writing data to frame at time %d:%d\n",
									   exe_name, address, request_page,
									   pcbt_shmptr[c_index].ptable[request_page].frame,
									   shmclock_shmptr->s, shmclock_shmptr->ns);

							pcbt_shmptr[c_index].ptable[request_page].dirty = 1;
						}
					}
					else
					{
						//Memory full...
						log(fpw, "%s: address (%d) [%d] is not in a frame, memory is full. Invoking page replacement...\n",
								   exe_name, address, request_page);

						//- Memory Management -//

						unsigned int lru_index = lru_stack->head->index;
						unsigned int lru_page = lru_stack->head->page;
						unsigned int lru_address = lru_page << 10;
						unsigned int lru_frame = lru_stack->head->frame;

						if (pcbt_shmptr[lru_index].ptable[lru_page].dirty == 1)
						{
							log(fpw, "%s: address (%d) [%d] was modified. Modified information is written back to the disk\n",
									   exe_name, lru_address, lru_page);
						}

						//Replacing procedure
						pcbt_shmptr[lru_index].ptable[lru_page].frame = -1;
						pcbt_shmptr[lru_index].ptable[lru_page].dirty = 0;
						pcbt_shmptr[lru_index].ptable[lru_page].valid = 0;

						pcbt_shmptr[c_index].ptable[request_page].frame = lru_frame;
						pcbt_shmptr[c_index].ptable[request_page].dirty = 0;
						pcbt_shmptr[c_index].ptable[request_page].valid = 1;

						//Update LRU stack and reference string
						list_remove(lru_stack, lru_index, lru_page, lru_frame);
						list_remove(reference_string, lru_index, lru_page, lru_frame);
						list_add(lru_stack, c_index, request_page, lru_frame);
						list_add(reference_string, c_index, request_page, lru_frame);

						//Modify dirty bit when requesting write of address
						if (pcbt_shmptr[c_index].ptable[request_page].protection == 1)
						{
							log(fpw, "%s: dirty bit of frame (%d) set, adding additional time to the clock\n", exe_name, last_frame);
							log(fpw, "%s: indicating to process (%d) [%d] that write has happend to address (%d) [%d]\n",
									   exe_name, master_message.spid, master_message.pid, address, request_page);
							pcbt_shmptr[c_index].ptable[request_page].dirty = 1;
						}
					}
				}
				else
				{
					//Update LRU stack
					int c_frame = pcbt_shmptr[c_index].ptable[request_page].frame;
					list_remove(lru_stack, c_index, request_page, c_frame);
					list_add(lru_stack, c_index, request_page, c_frame);

					//Giving data to process OR writing data to frame
					if (pcbt_shmptr[c_index].ptable[request_page].protection == 0)
					{
						log(fpw, "%s: address (%d) [%d] is already in frame (%d), giving data to process (%d) [%d] at time %d:%d\n",
								   exe_name, address, request_page,
								   pcbt_shmptr[c_index].ptable[request_page].frame,
								   master_message.spid, master_message.pid,
								   shmclock_shmptr->s, shmclock_shmptr->ns);
					}
					else
					{
						log(fpw, "%s: address (%d) [%d] is already in frame (%d), writing data to frame at time %d:%d\n",
								   exe_name, address, request_page,
								   pcbt_shmptr[c_index].ptable[request_page].frame,
								   shmclock_shmptr->s, shmclock_shmptr->ns);
					}
				} //END OF: page_table.valid
			}	  //END OF: master_message.flag

			//Point the pointer to the next queue element
			qnext.next = (qnext.next->next != NULL) ? qnext.next->next : NULL;

			//Reset master message
			master_message.type = -1;
			master_message.spid = -1;
			master_message.pid = -1;
			master_message.flag = -1;
			master_message.page = -1;
		} //END OF: qnext.next

		//--------------------------------------------------
		//Reassigned the current queue
		while (!queue_empty(queue))
		{
			queue_pop(queue);
		}
		while (!queue_empty(trackingQueue))
		{
			int i = trackingQueue->front->index;
			queue_push(queue, i);
			queue_pop(trackingQueue);
		}
		free(trackingQueue);

		//- CRITICAL SECTION -//
		incShmclock(0);

		//--------------------------------------------------
		//Check to see if a child exit, wait no bound (return immediately if no child has exit)
		int child_status = 0;
		pid_t child_pid = waitpid(-1, &child_status, WNOHANG);

		//Set the return index bit back to zero (which mean there is a spot open for this specific index in the bitmap)
		if (child_pid > 0)
		{
			int return_index = WEXITSTATUS(child_status);
			bitmap[return_index / 8] &= ~(1 << (return_index % 8));
		}

		//--------------------------------------------------
		//End the infinite loop when reached X forking times. Turn off timer to avoid collision.
		if (fork_number >= TOTAL_PROCESS)
		{
			timer(0);
			masterHandler(0);
		}
	} //END OF: infinite while loop #1

	return EXIT_SUCCESS;
}

void log(FILE *fpw, char *fmt, ...)
{
	char buf[BUFFER_LENGTH];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	if (isDisplayTerminal)
	{
		fprintf(stderr, buf);
	}

	if (fpw != NULL)
	{
		fprintf(fpw, buf);
		fflush(fpw);
	}
}

void masterInterrupt(int seconds)
{
	//Invoke timer for termination
	timer(seconds);

	//Signal Handling for: SIGALRM
	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &masterHandler;
	sa1.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa1, NULL) == -1)
	{
		perror("ERROR");
	}

	//Signal Handling for: SIGINT
	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &masterHandler;
	sa2.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa2, NULL) == -1)
	{
		perror("ERROR");
	}

	//Signal Handling for: SIGUSR1
	signal(SIGUSR1, SIG_IGN);

	//Signal Handling for: SIGSEGV
	signal(SIGSEGV, segHandler);
}
void masterHandler(int signum)
{
	if (!isDisplayTerminal)
	{
		fprintf(stderr, "%8s(%d/%d)\n\n", "", fork_number, TOTAL_PROCESS);
	}
	finalize();

	//Print out basic statistic
	double mem_p_sec = (double)memoryaccess_number / (double)shmclock_shmptr->s;
	double pg_f_p_mem = (double)pagefault_number / (double)memoryaccess_number;
	double avg_m = (double)total_access_time / (double)memoryaccess_number;
	avg_m /= 1000000.0;

	log(fpw, "- Master PID: %d\n", getpid());
	log(fpw, "- Number of forking during this execution: %d\n", fork_number);
	log(fpw, "- Final simulation time of this execution: %d.%d\n", shmclock_shmptr->s, shmclock_shmptr->ns);
	log(fpw, "- Number of memory accesses: %d\n", memoryaccess_number);
	log(fpw, "- Number of memory accesses per nanosecond: %f memory/second\n", mem_p_sec);
	log(fpw, "- Number of page faults: %d\n", pagefault_number);
	log(fpw, "- Number of page faults per memory access: %f pagefault/access\n", pg_f_p_mem);
	log(fpw, "- Average memory access speed: %f ms/n\n", avg_m);
	log(fpw, "- Total memory access time: %f ms\n", (double)total_access_time / 1000000.0);
	fprintf(stderr, "SIMULATION RESULT is recorded into the log file: %s\n", log_file);

	cleanUp();

	//Final check for closing log file
	if (fpw != NULL)
	{
		fclose(fpw);
		fpw = NULL;
	}

	exit(EXIT_SUCCESS);
}
void segHandler(int signum)
{
	fprintf(stderr, "Segmentation Fault\n");
	masterHandler(0);
}
void exitHandler(int signum)
{
	fprintf(stderr, "%d: Terminated!\n", getpid());
	exit(EXIT_SUCCESS);
}

void timer(int seconds)
{
	struct itimerval value;
	value.it_value.tv_sec = seconds;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;

	if (setitimer(ITIMER_REAL, &value, NULL) == -1)
	{
		perror("ERROR");
	}
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
	if (mqueueid > 0)
	{
		msgctl(mqueueid, IPC_RMID, NULL);
	}

	discardShm(shmclock_shmid, shmclock_shmptr, "shmclock", exe_name, "Master");

	if (semid > 0)
	{
		semctl(semid, 0, IPC_RMID);
	}

	discardShm(pcbt_shmid, pcbt_shmptr, "pcbt", exe_name, "Master");
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

int incShmclock(int increment)
{
	semaLock(0);
	int nano = (increment > 0) ? increment : rand() % 1000 + 1;

	forkclock.ns += nano;
	shmclock_shmptr->ns += nano;

	while (shmclock_shmptr->ns >= 1000000000)
	{
		shmclock_shmptr->s++;
		shmclock_shmptr->ns = abs(1000000000 - shmclock_shmptr->ns);
	}

	semaRelease(0);
	return nano;
}

void initPCBT(PCB *pcbt)
{
	int i, j;
	for (i = 0; i < MAX_PROCESS; i++)
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