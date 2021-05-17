// Program to illustrate the use of system calls to create a process and execute
// a command, as a shell will do it.

// Written by: Sanjiv K. Bhatia

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main()
{
    pid_t childpid, cpid;		// Process id for child
    char command[] = "ls";		// Command to be executed
    char path[] = "/bin/ls";		// Path for the command
    int status;				// Return value from child

    // Fork child

    if ( ( childpid = fork() ) < 0 )
    {
	perror ( "Error forking child" );
	return ( 1 );			// Terminate code
    }

    if ( childpid )			// Code for parent
    {
	printf ( "In parent\n" );
        printf ( "Process id %ld Parent id %ld\n", (long) getpid(), (long) getppid() );
	printf ( "Waiting for child to terminate ...\n" );

	cpid = wait ( &status );

	printf ( "Child %ld returned %d\n", cpid, WEXITSTATUS(status) );

	return ( 0 );			// Parent terminates
    }

    // This is the code for child

    sleep ( 10 );		// Wait for 10 sec before executing child
    execl ( path, command, (char*)(NULL) );

    // We come to this point only if execl failed.

    perror ( "Problem with execl: " );

    return ( 1 );
}

