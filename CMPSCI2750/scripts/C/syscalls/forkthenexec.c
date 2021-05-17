#include <stdio.h>
#include <unistd.h>

int main()
{
    pid_t	pid;
    printf ( "Start of test\n" );
    pid = fork();
    if ( pid )
    {
    	printf ( "Returned pid is: %d\n", pid );
	return ( 0 );
    }

    // Child process
    
    execl ( "/usr/bin/ls", "ls", NULL );
    printf ( "Error in execl\n" );
}
