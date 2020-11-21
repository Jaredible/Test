/*
 * shared.h November 21, 2020
 * Jared Diehl (jmddnb@umsystem.edu)
 */

#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>

#define log _log

#define BUFFER_LENGTH 1024
#define FILE_LENGTH_MAX 100000

#define KEY_PATHNAME "."
#define KEY_ID_CLOCK 0
#define KEY_ID_RESOURCE_DESCRIPTOR 1
#define KEY_ID_MESSAGE_QUEUE 2
#define PERMS (S_IRUSR | S_IWUSR)

#define PATH_LOG "./output.log"
#define TIMEOUT 10
#define PROCESSES_MAX 18
#define PROCESSES_TOTAL 40
#define RESOURCES_MAX 20
#define SHARED_RESOURCES_MIN (int) (RESOURCES_MAX * 0.15)
#define SHARED_RESOURCES_MAX (int) (RESOURCES_MAX * 0.25)

enum ActionType { TERMINATE, REQUEST, RELEASE };

typedef struct {
	unsigned int s;
	unsigned int ns;
} Time;

typedef struct {
	long mtype;
	int index;
	pid_t childPid;
	int flag;
	bool isRequest;
	bool isRelease;
	bool isSafe;
	char message[BUFFER_LENGTH];
} Message;

typedef struct {
	int init[RESOURCES_MAX];
	int resource[RESOURCES_MAX];
	int shared;
} Data;

typedef struct {
	pid_t pid;
	int spid;
	int maximum[RESOURCES_MAX];
	int allocation[RESOURCES_MAX];
	int request[RESOURCES_MAX];
	int release[RESOURCES_MAX];
} PCB;

#endif