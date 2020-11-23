#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>

typedef unsigned int uint;

#define BUFFER_LENGTH 4096
#define MAX_FILE_LINE 100000

#define TERMINATION_TIME 20
#define MAX_PROCESS 18
#define TOTAL_PROCESS 100

#define PROCESS_SIZE 32000
#define PAGE_SIZE 1000
#define MAX_PAGE (PROCESS_SIZE / PAGE_SIZE)

#define MEMORY_SIZE 256000
#define FRAME_SIZE PAGE_SIZE
#define MAX_FRAME (MEMORY_SIZE / FRAME_SIZE)

typedef struct {
	unsigned int s;
	unsigned int ns;
} Time;

typedef struct {
	long type;
	pid_t pid;
	int spid;
	int flag;
	unsigned int address;
	unsigned int page;
	char text[BUFFER_LENGTH];
} Message;

typedef struct {
	uint frame;
	uint address: 8;
	uint protection: 1;
	uint dirty: 1;
	uint valid: 1;
} PTE;

typedef struct {
	int spid;
	pid_t pid;
	PTE ptable[MAX_PAGE];
} PCB;

#endif