/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: shared.h
# Date: 10/30/2019
==================================================================================================== */
#ifndef MY_SHARED_H
#define MY_SHARED_H
#include <stdbool.h>
#include "constant.h"

/* File related constant */
#define BUFFER_LENGTH 4096
#define MAX_FILE_LINE 100000


/* Process related constant */
//Note (Warning): MAX_PROCESS should not be greater than 18 or 28 (depend on your setting)
#define TERMINATION_TIME 5
#define MAX_PROCESS 18
#define TOTAL_PROCESS 100


/* Paging related constant */
//Note: page is your program (processes) divide into smaller block
//Note: each process will have a requirement of less than 32K memory, with each page being 1K
//Note: each process has 32 pages (each processes can have up to 32 frames at given time)
#define PROCESS_SIZE 32000
#define PAGE_SIZE 1000
#define MAX_PAGE (PROCESS_SIZE / PAGE_SIZE)

//Note: frame is your memory didvide into smaller block
//Note: your system has a total memory of 256K
//Note: you have 256 frames available
//Note: a problem occur when total memory exceed 100k
#define MEMORY_SIZE 256000
#define FRAME_SIZE PAGE_SIZE
#define MAX_FRAME (MEMORY_SIZE / FRAME_SIZE)


/* New type variable */
typedef unsigned int uint;


/* SharedClock */
typedef struct 
{
	unsigned int second;
	unsigned int nanosecond;
}SharedClock;


/* Message */
typedef struct
{
	long mtype;
	int index;
	pid_t childPid;
	int flag;	//0 : isDone | 1 : isQueue
	unsigned int address;
	unsigned int requestPage;
	char message[BUFFER_LENGTH];
}Message;


/* PageTableEntry */
typedef struct
{
	uint frameNo;
	uint address: 8;
	uint protection: 1; //What kind of protection you want on that page, such as read only or read and write. (0 : read only | 1 : read and write) 
	uint dirty: 1;      //Is set if the page has been modified or written into. (0 : nay | 1 : aye)
	uint valid: 1;      //Indicate wheter the page is in memory or not. (0: nay | 1 : aye)
}PageTableEntry; 

/* ProcessControlBlock */
typedef struct
{
	int pidIndex;
	pid_t actualPid;
	PageTableEntry page_table[MAX_PAGE];
}ProcessControlBlock;


#endif

