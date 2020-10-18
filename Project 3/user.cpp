/*

        Author: Daniel Janis
        Program: Project 2 - Practice Shared Memory and IPC - CS 4760-002
    Date: 10/17/20
    File: user.cpp
    Purpose:

*/

#include <cstring>
#include <ctime>
#include <time.h>
#include <string>
#include <ctype.h>
#include "user.h"
#include "shared.h"

struct Shmem* shmem; // struct instance for shared memory
struct Msgbuf buf1, buf2; // struct instance for message queue

key_t skey; // Unique key for using shared memory
int sid; // Shared memory segment ID

key_t mkey_send, mkey_rec; // Unique key for using message queue
int mqid_send, mqid_rec; // Message queue segment ID

int main(int argc, char *argv[]) {

    //int index = atoi(argv[0]);

    //srand(time(NULL)); // Seed the random number generator once for the sleep() later on

    std::string error_msg; // For perror's

    // Checks if the process was killed by somebody with the *kill* program, if found, calls sig_handler()
    signal(SIGTERM, sig_handler);

    // Generate a (key_t) type System V IPC key for use with shared memory
    if ((skey = ftok("makefile", 'a')) == (key_t) -1) {
        perror("user: ftok: Error: There was an error creating a shared memory key");
        exit(EXIT_FAILURE);
    }

    // Tries to allocate a shared memory segment, otherwise it will use the
    // previously allocated shared memory segment
    if ((sid = shmget(skey, sizeof(struct Shmem), 0/*IPC_CREAT | 0666*/)) < 0) {
        // if the sid is < 0, it couldn't allocate a shared memory segment
        perror("user: shmget: Error: An error occurred while trying to allocate a valid shared memory segment");
        exit(EXIT_FAILURE);
    }
    else {
        shmem = (struct Shmem*) shmat(sid, NULL, 0);
        // attaches to Sys V shared mem segment using previously allocated memory segment (sid)
        // Since shmaddr is NULL, system chooses a suitable (unused) page-aligned address to attach the segment
    }


    //////////////////////////
    /* SET UP MESSAGE QUEUE */
    //////////////////////////


    // Generate a (key_t) type System V IPC key for use with message queue
    if ((mkey_send = ftok("makefile", 'b')) == (key_t) -1) {
        error_msg = "user: Message Queue: ftok: Error: there was an error creating a message queue key";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }

    // Connect to the queue (sending queue)
    if ((mqid_send = msgget(mkey_send, 0644)) == -1) {
        error_msg = "user: Message Queue: msgget: Error: Cannot allocate a valid message queue";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }

    // Generate a (key_t) type System V IPC key for use with message queue
    if ((mkey_rec = ftok("makefile", 'c')) == (key_t) -1) {
        error_msg = "user: Message Queue: ftok: Error: there was an error creating a message queue key";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }

    // Connect to the queue (receiving queue)
    if ((mqid_rec = msgget(mkey_rec, 0644)) == -1) {
        error_msg = "user: Message Queue: msgget: Error: Cannot allocate a valid message queue";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }

    /////////////////////////////
    /* SET UP CRITICAL SECTION */
    /////////////////////////////


    // USER receiving PID from OSS (buf1.mflag)
    if (msgrcv(mqid_send, &buf1, sizeof(buf1), 0, 0) == -1) {
        error_msg = "user: msgrcv: Error: Message was not received";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }

      /* ONCE THE MESSAGE ABOVE IS RECEIVED, WE ENTER THE CRITICAL SECTION */


    // TIME TO TERMINATE IS DETERMINED BELOW
    srand(getpid()+buf1.mflag);
    int current_sec = shmem->sec;
    int current_nanosec = shmem->nanosec;
    int random = (rand() % 50000000) + 1;/*1; (keep this 1 for FINAL VERSION) */ // Sets random to a random integer 1-1000000 nanoseconds (1ns - 1ms)
    current_nanosec += random; // ADDS THE RANDOM AMOUNT OF NANOSECONDS - this guarantess the time it *should* terminate
    if (current_nanosec >= 1000000000) { // Convert nanoseconds to seconds if possible
        current_nanosec -= 1000000000; // Minus 1 second (1 billion nanoseconds)
        current_sec += 1; // Add 1 second
    }

    // BASICALLY, if the shared clock goes over my calculated "time to terminate", then look at the PID
    while (!(shmem->sec > current_sec) && !((shmem->sec == current_sec) && (shmem->nanosec > current_nanosec))) {
        buf2.mflag = 2; // from child to master and we are not done
        if (msgsnd(mqid_rec, &buf2, sizeof(buf2), IPC_NOWAIT) < 0) {
            error_msg = "user: msgsnd: Error: Message was not sent";
            perror(error_msg.c_str());
            exit(EXIT_FAILURE);
        }
        // Checked time and it is not time to terminate yet
        // Waiting for master to say its my turn again
        if (msgrcv(mqid_send, &buf1, sizeof(buf1), 0, 0) == -1) {
            error_msg = "user: msgrcv: Error: Message was not received";
            perror(error_msg.c_str());
            exit(EXIT_FAILURE);
        }
    }


    while (shmem->shmPID != 0) {} // waits for a different child to say its done OR parent response
    // this loops until NO children are in the critical section, opens it up for a new child below

    shmem->shmPID = getpid(); // Grabs the PID of the child that is now to terminate

    // ONCE THE MESSAGE BELOW IS RECEIVED IN OSS, WE ARE EXITING THE CRITICAL SECTION
    buf2.mflag = 3; // message to be sent from USER to OSS (LEAVING CRITICAL SECTION)
    if (msgsnd(mqid_rec, &buf2, sizeof(buf2), IPC_NOWAIT) < 0) { // Sends message on buf2 from USER to OSS
        error_msg = "user: msgsnd: Error: Message was not sent";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }

    shmdt(shmem); // Detaches shared memory pointer from this child process

    return 0;
}

void sig_handler(int signal) { // CLOSES ANY RUNNING CHILDREN WHEN CTRL+C IS PRESSED, or when timer runs out
    if (signal == 15) {
        printf("[USER]: %d -- Process forced to terminate!\n", getpid());
        //shmdt(shmem);
        exit(EXIT_FAILURE);
    }
}

// This function gets the local time at the time of calling
char *get_time() {
    time_t seconds = time(0); // Holds the current time from system
    struct tm * formatted = localtime(&seconds); // gives local time representation of timer
    char* ch_time = new char[12]; // Allocates new memory for a char ptr array
    strftime(ch_time, 12, "%I:%M:%S %p", formatted); // Store the formatted time
    return (char*) ch_time; // Returns a formatted time to be printed
}