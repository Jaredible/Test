// Code to illustrate the use of system errors by opening a file

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE * open_file ( char * fn )
{
    FILE * fd;

    errno = 0;			// Automatically allocated from errno.h
    fd = fopen ( fn, "r" );
    if ( ! fd )
    {
	fprintf (stderr, "%s: Couldn't open file %s; %s\n",
//	    program_invocation_short_name,	// Contains base name of program
	    program_invocation_name,		// Contains full path used
	    fn,
	    strerror ( errno )		// Convert error number to string
	    );
	exit ( EXIT_FAILURE );
    }
    else
	return ( fd );
}

int main()
{
    FILE * fd = open_file ( "foobar" );
    close ( fd );
    return ( 0 );
}
