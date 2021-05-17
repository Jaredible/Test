#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

int main()
{
    int		errno;		// To catch error number, in case fork fails
    int		i;		// Counter for for-loop
    int		status;		// Status returned from child
    pid_t	pid;		// Process id
    pid_t	rpid;		// Returned pid from child

    FILE * fn = fopen ( "foobar", "w" );	// Open a file to write
    if ( ! fn )
    {
	fprintf ( stderr, "Error opening file foobar\n" );
	return ( 1 );
    }

    // Make sure that the file is unbuffered.

    setvbuf ( fn, NULL, _IONBF, 0 );

    pid = fork();				// Create a child process

    if ( pid < 0 )
    {
	perror ( "Error: " );
	fclose ( fn );
	return ( 1 );
    }

    // Generate random number seed separately in both parent and child.
    // The 2 second sleep is introduced to change the random number seed;
    // otherwise, the two processes generate the same random number sequence

    if ( pid )
    {
	srandom ( time ( NULL ) );
	sleep ( 2 );
    }
    else
    {
	sleep ( 2 );
	srandom ( time ( NULL ) );
    }

    for ( i = 0; i < 10; i++ )
    {
	int r = random () % 5;		// Random number between 0 and 4.
	sleep ( r );			// Sleep for random amount
	fprintf ( fn, "Printing %2d after sleeping for %d sec in %s",
		i, r, pid ? "parent\n" : "child\n" );
    }

    fprintf ( fn, "%s finished; closing file\n", pid ? "Parent" : "Child" );
    fclose ( fn );		// Close file in both parent and child

    // Parent waits for child to finish

    if ( pid )
    {
	rpid = wait ( &status );

	if ( rpid < 0 )
	{
	    perror ( "Error in wait: " );
	    return ( 1 );
	}
    }

    return ( 0 );
}

