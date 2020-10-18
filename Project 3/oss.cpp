/*

        Author: Daniel Janis
        Program: Project 2 - Practice Shared Memory and IPC - CS 4760-002
        Date: 10/17/20
    File: oss.cpp
        Purpose:

*/

#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include "oss.h"
#include "shared.h"

static pid_t gpid; // Group Process ID
static int dead;

int pr_limit = 100; // max total of child processes master will ever create [-n x] (Default: 4)
int ch_limit = 5; // number of concurrent children allowed to exist in the system at the same time [-s x] (Default: 5)
int timer = 20; // time in seconds after which the process will terminate, even if it has not finished [-t time] (Default: 100)
int pr_count = 0; // Current running total of children processes present

struct Shmem* shmem; // struct instance used for shared memory
struct Msgbuf buf1, buf2; // struct instance used for message queue

key_t skey; // Unique key for using shared memory
int sid; // Shared memory segment ID

key_t mkey_send, mkey_rec; // Unique key for using message queue
int mqid_send, mqid_rec; // Message queue ID

int main(int argc, char *argv[]) {

    // Checks for interrupt from keyboard, if found, calls sig_handle()
    signal(SIGINT, sig_handle);

    //int word_count = 0; // stores the total # of words in the file passed in

    // Used to store the name of the input file
    std::string logfile;

    // If there is an error message, this helps us pass it
    std::string error_msg;

    // Remove the ./ from the command-line invokation
    std::string exe_name;
    exe_name = argv[0];
    std::string ext = "/";
    if (exe_name.find(ext) != -1) {
        size_t findExt = exe_name.find_last_of("/");
        exe_name = exe_name.substr(findExt+1, exe_name.length());
    } // exe_name is the name of program that was called

    // This while loop + switch statement allows for the checking of parse options
    int opt;
    while ((opt = getopt(argc, argv, "c:l:t:h")) != -1) {
        switch (opt) {
            case 'c': // Number of children allowed to exist in system at the same time
                ch_limit = atoi(optarg);
                // Checks that the number of children allowed is positive, nonzero
                if (ch_limit <= 0 || ch_limit > 27) {
                    error_msg = "[-c x] value should be within 1-27 processes at the same time.";
                    errors(exe_name.c_str(), error_msg.c_str());
                }
                break;
            case 'l':
                logfile = optarg;
                break;
            case 't': // where time is the max time a process will run before terminating
                timer = atoi(optarg);
                // Checks that the timer is positive and nonzero
                if (timer <= 0) {
                    error_msg = "[-t time] value should be positive and nonzero.";
                    errors(exe_name.c_str(), error_msg.c_str());
                }
                break;
            case 'h': // -h will describe how the project should be run and then terminate
            default:
                usage(exe_name.c_str());
        }
    }
    // Check that the logfile can be opened, otherwise give it a default name
    FILE *logptr;
    logptr = fopen(logfile.c_str(), "w"); // overwrites the file if it exists, or creates a new file
    if (logptr == NULL) { // Checks that the file is able to be opened
        logfile = "logfile.log"; // Default log file
    }
    else {
        fclose(logptr); // close the fresh log file
    }

    printf("\n______________________\n");
    printf("\n child limit: %d\n     logfile: %s\n       timer: %d\n", ch_limit, logfile.c_str(), timer);
    printf("______________________\n\n");
    if (argv[optind] != NULL) { // Makes sure that no extra command-line options were passed
        error_msg = "Too many arguments were passed, check the usage line below.";
        errors(exe_name.c_str(), error_msg.c_str());
    }

    /////////////////////////////////////////
    /* ALLOCATE OR ATTACH TO SHARED MEMORY */
    /////////////////////////////////////////

    // Generate a (key_t) type System V IPC key for use with shared memory
    if ((skey = ftok("makefile", 'a')) == (key_t) -1) {
        error_msg = exe_name + ": Shared Memory: ftok: Error: there was an error creating a shared memory key";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }

    // Tries to allocate a shared memory segment, otherwise it will use the
    // previously allocated shared memory segment
    if ((sid = shmget(skey, sizeof(struct Shmem), IPC_CREAT | 0666)) < 0) {
        // if the sid is < 0, it couldn't allocate a shared memory segment
        error_msg = exe_name + ": Shared Memory: shmget: Error: An error occurred while trying to allocate a valid shared memory segment";
        perror(error_msg.c_str());
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
        error_msg = exe_name + ": Message Queue: ftok: Error: there was an error creating a message queue key";
        perror(error_msg.c_str());
        free_memory();
        exit(EXIT_FAILURE);
    }

    // Creates a System V message queue (from OSS to USER)
    if ((mqid_send = msgget(mkey_send, 0644 | IPC_CREAT)) < 0) {
        error_msg = exe_name + ": Message Queue: msgget: Error: Cannot allocate a valid message queue";
        perror(error_msg.c_str());
        free_memory();
        exit(EXIT_FAILURE);
    }

    // Generate a (key_t) type System V IPC key for use with message queue
    if ((mkey_rec = ftok("makefile", 'c')) == (key_t) -1) {
        error_msg = exe_name + ": Message Queue: ftok: Error: there was an error creating a message queue key";
        perror(error_msg.c_str());
        free_memory();
        exit(EXIT_FAILURE);
    }

    // Creates a System V message queue (from USER to OSS)
    if ((mqid_rec = msgget(mkey_rec, 0644 | IPC_CREAT)) < 0) {
                error_msg = exe_name + ": Message Queue: msgget: Error: Cannot allocate a valid message queue";
        perror(error_msg.c_str());
        free_memory();
        exit(EXIT_FAILURE);
    }


    ///////////////////////////
    /* START COUNTDOWN TIMER */
    ///////////////////////////

    countdown_to_interrupt(timer, exe_name.c_str());


    //////////////////////////////////////////////////
    /* INITIALIZE THE SHARED MEMORY CLOCK */
    //////////////////////////////////////////////////

    // Initialize shared memory clock to 0
    shmem->sec = 0;
    shmem->nanosec = 0;

    // Initialize the shared memory int that indicates when child processes terminate
    shmem->shmPID = 0;

    dead = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    /* fork off appropriate number of child processes until the termination criteria is met */
    //////////////////////////////////////////////////////////////////////////////////////////

    pid_t childpid; // child process to be
    int running_procs = 0;

    for (int i = 0; i < ch_limit; i++) {
        childpid = fork(); // Returns 0 if child created,
        if (childpid == 0) { // Returns to the newly creates child process
            if (running_procs == 0) { // CREATES A PROCESS GROUP FOR LATER (Safely terminate all children on CTRL+C)
                shmem->pgid = getpid();
            }
            setpgid(0, shmem->pgid);
            execl("./user", NULL); // Execute the "user" executable on the new child process
            error_msg = exe_name + ": Error: Failed to execl";
            perror(error_msg.c_str());
            exit(EXIT_FAILURE);
        }
        ++pr_count; // a process has been started, total counter incremented
        ++running_procs; // a new process is now running, keep track of how many are also running
    }
    while(1) {
        if (shmem->shmPID != 0) { // Back in the parent and the PID of the child was set, (child has ended)
            fprintf(stderr, "[OSS] process counter: %d\n", pr_count);

            /* LOG THAT THE CHILD PROC HAS TERMINATED AT THE TIME ON SHARED CLOCK */
            //++dead;
            fprintf(stderr, "[OSS] %d -- total dead (pre-increment): %d -- terminated -- at %d.%09d seconds\n", shmem->shmPID, dead, shmem->sec, shmem->nanosec);
            fprintf(stderr, "[OSS] %d -- Process Group ID\n", shmem->pgid);
            waitpid(shmem->shmPID, NULL, 0);
            shmem->shmPID = 0;
            ++dead;
            fprintf(stderr, "[OSS] %d -- Process Group ID -- total dead (post-increment): %d\n", shmem->pgid, dead);
            --running_procs;
        }
        else { // Inrement timer because child process is still executing
            shmem->nanosec += 500000000; // Increment nanoseconds by 100
            if (shmem->nanosec >= 1000000000) {
               shmem->nanosec -= 1000000000;
               shmem->sec += 1;
            }
            /*if (shmem->sec >= 2) { // When 2 seconds elapsed has passed,
                fprintf(stderr, "[OSS] elapsed time: %d.%d\n", shmem->sec, shmem->nanosec);
                sig_handle(SIGTERM);
                break;
            }*/
        }


        // If running_procs is 1 less than the limit then the next if block will create a process,
        // if pr_count is 99, the next process will be #100,
        // If both of the above are true, then this stops just before process 100 gets created!

        if (pr_count+1 == 100 /*&& running_procs == 0*/ && running_procs == ch_limit-1) { // HAVE WE HIT THE LIMIT OF 100 PROCESSES
            // termination criteria (STILL NEED TO LET THE CHILD PROCESSES FINISH)
            fprintf(stderr, "[OSS] process limit: %d, number of currently running procs: %d\n[OSS]: exiting OSS! Goodbye!\n", pr_count, running_procs);
            fprintf(stderr, "dead: %d\n", dead);
            //signal(SIGINT, sig_handle);
            sig_handle(SIGTERM);
            //sleep(20);
            break;
        }
        if ((running_procs < ch_limit) && (pr_count < 100)) { // loop stops if 100 processes have ran or concurrent
            childpid = fork(); // Returns 0 if child created,
            if (childpid == 0) { // Returns to the newly creates child process
                if (ch_limit == 1/* || running_procs == 0*/) { // If only one concurrent process was allowed, this makes sure it can terminate successfully
                    shmem->pgid = getpid();
                }
                setpgid(0, shmem->pgid);
                execl("./user", NULL); // Execute the "user" executable on the new child process
                error_msg = exe_name + ": Error: Failed to execl";
                perror(error_msg.c_str());
                exit(EXIT_FAILURE);
            }
            ++pr_count; // a process has been started, total counter incremented
            ++running_procs; // a new process is now running, keep track of how many are also running
        }

        //fprintf(stderr, "[OSS] %d -- LEAVING CS\n", shmem->shmPID);
        static int count = 1;
        buf1.mflag = count;
        count++;
        if (msgsnd(mqid_send, &buf1, sizeof(buf1), IPC_NOWAIT) < 0) {
            error_msg = exe_name + ": Error: msgsnd: the message did not send";
            perror(error_msg.c_str());
            exit(EXIT_FAILURE);
        }
        if (msgrcv(mqid_rec, &buf2, sizeof(buf2), 0, 0) == -1) {
            error_msg = "oss: msgrcv: Error: Message was not received";
            perror(error_msg.c_str());
            exit(EXIT_FAILURE);
        }
    }

    free_memory(); // Clears all shared memory
    return 0;
}

// Kills all child processes and terminates, and prints a log to log file and frees shared memory
void sig_handle(int signal) {
    //printf("signal: %d\n", signal);
    if (signal == 2) {
        printf("\n[OSS]: CTRL+C was received, interrupting process!\n");
    }
    if (signal == 14) { // "wake up call" for the timer being done
        printf("\n[OSS]: The countdown timer [-t time] has ended, interrupting processes!\n");
    }
    if (signal = 15) {
        printf("\n[OSS]: 100 Processes reached, interrupting processes!\n", signal);
    }
    if (killpg(shmem->pgid, SIGKILL/*SIGTERM*/) == -1) {
        fprintf(stderr, "\n[OSS]: One of my child processes did not have its group PID set. This means I could not terminate it. Woops.\n");
        free_memory();
        kill(getpid(), SIGKILL/*SIGTERM*/);
    } // Sends a kill signal to the child process group (safely kills all children)
    //sleep(1); // Fixes an error I was getting where THE FINAL 100th CHILD PROCESS would try and attach to shared memory,
              // after the free_memory() function ran here in the parent process. This sleep(1) allows for the processes
              // to be fully terminated before freeing shared memory.
    while(wait(NULL) > 0);
    free_memory(); // clears all shared memory
    exit(0);
}

/*
struct sigaction {
    void (*sa_handler) (int);
    void (*sa_sigaction) (int, siginfo_t *, void *);
    sigset_t sa_mask; // specifies a mask of signals which should be blocked during execution of
                      // the signal handling function. The signal which triggers the handler gets blocked too
    int sa_flags;
    void (*sa_restorer) (void);
};

This struct is shown on the sigaction(2) manual page and it can be
modified to change the action taken by a process on receipt of a
specific signal, used below it is able to send a signal to sig_handle()
whenever the timer (given by option [-t time]) runs out of time.  */

// Countdown timer based on [-t time] in seconds
void countdown_to_interrupt(int seconds, std::string exe_name) {
    std::string error_msg;
    clock(seconds, exe_name.c_str());
    struct sigaction action; // action created using struct shown above
    sigemptyset(&action.sa_mask); // Initializes the signal set to empty
    action.sa_handler = &sig_handle; // points to a signal handling function,
              // which specifies the action to be associated with SIGALRM in sigaction()
    action.sa_flags = SA_RESTART; // Restartable system call
    // SIGALRM below is an asynchronous call, the signal raised when the time interval specified expires
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        // &action specifies the action to be taken when the timer is done
        // NULL means that the previous, old action, isn't saved
        error_msg = exe_name + ": Error: Sigaction structure was not initialized properly";
        perror(error_msg.c_str());
        exit(EXIT_FAILURE);
    }
}

// This is used to create a clock, helper function for countdown_to_interrupt()
void clock(int seconds, std::string exe_name) {
    std::string error_msg;
    struct itimerval time;
    // .it_value = time until timer expires and signal gets sent
    time.it_value.tv_sec = seconds; // .tv_sec = configure timer to "seconds" whole seconds
    time.it_value.tv_usec = 0; // .tv_usec = configure timer to 0 microseconds
    // .it_interval = value the timer will be set to once it expires and the signal gets sent
    time.it_interval.tv_sec = 0; // configure the interval to 0 whole seconds
    time.it_interval.tv_usec = 0; // configure the interval to 0 microseconds
    if (setitimer(ITIMER_REAL, &time, NULL) == -1) { // makes sure that the timer gets set
        error_msg = exe_name + ": Error: Cannot arm the timer for the requested time";
        perror(error_msg.c_str());
    } // setitimer(ITIMER_REAL, ..., NULL) is a timer that counts down in real time,
      // at each expiration a SIGALRM signal is generated
}


// Prints an error message based on the calling executable
void errors(std::string name, std::string message) {
    printf("\n%s: Error: %s\n\n", name.c_str(), message.c_str());
    usage(name);
}

// Prints a usage message about how to properly use this program
void usage(std::string name) {
    printf("\n%s: Usage: ./master [-c x] [-l filename] [-t z]\n", name.c_str());
    printf("%s: Help:  ./master -h\n                    [-h] will display how the project should be run and then terminate.\n", name.c_str());
    printf("    [-c x] where x is the number of children allowed to exist at one time in the system. (Default 5)\n");
    printf("    [-t z] where z is the max time you want the program to run before terminating. (Default 20)\n");
    printf("    [-l filename] is the name of the input file containing strings to be tested.\n\n");
    exit(EXIT_FAILURE);
}

// Takes the shared memory struct and frees the shared memory
void free_memory() {

    // Performs control operations on the sending message queue
    if (msgctl(mqid_send, IPC_RMID, NULL) == -1) { // Removes the message queue
        //error_msg = exe_name + ": Message Queue: msgctl: Error: Cannot remove message queue";
        perror("oss: Error: Could not remove the mqid_send message queue");
        exit(EXIT_FAILURE);
    }

    // Performs control operations on the receiving message queue
    if (msgctl(mqid_rec, IPC_RMID, NULL) == -1) { // Removes the message queue
        //error_msg = exe_name + ": Message Queue: msgctl: Error: Cannot remove message queue";
        perror("oss: Error: Could not remove the mqid_rec message queue");
        exit(EXIT_FAILURE);
    }

    shmdt(shmem); // Detaches the shared memory of "shmem" from the address space of the calling process
    shmctl(sid, IPC_RMID, NULL); // Performs the IPC_RMID command on the shared memory segment with ID "sid"
    // IPC_RMID -- marks the segment to be destroyed. This will only occur after the last process detaches it.
    printf("[OSS]: all shared memory and message queues freed up! terminating!\n");
}
