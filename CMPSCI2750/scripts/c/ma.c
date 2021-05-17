#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IN
#define OUT
#define INOUT

int makeargv (
    IN	const char *   s, 		// String to be parsed
    IN	const char *   delimiters,	// String of delimiters
    OUT       char *** argvp		// Argument array
    )
{
    int		error;
    int		i;
    int		numtokens;
    const char * snew;
    char       * t;

    // Make sure that none of the inputs is NULL

    if ( ! ( s && delimiters && argvp ) )
    {
	errno = EINVAL;
	return ( -1 );
    }

    *argvp = NULL;
    snew = s + strspn ( s, delimiters );	// Real start of string

    t = ( char * ) malloc ( strlen ( snew ) + 1 );
    if ( !t )
    {
	fprintf ( stderr, "Error allocating memory\n" );
	return ( -1 );
    }

    strcpy ( t, snew );
    numtokens = 0;

    // Count the number of tokens

    if ( strtok ( t, delimiters ) )
	for ( numtokens = 1; strtok ( NULL, delimiters ); numtokens++ );

    // Create argument array for pointers to tokens

    *argvp = ( char ** ) malloc ( ( numtokens + 1 ) * sizeof ( char * ) );
    error = errno;
    if ( ! *argvp )
    {
	free ( t );
	errno = error;
	return ( -1 );
    }

    if ( ! numtokens )
	free ( t );
    else
    {
	strcpy ( t, snew );
	**argvp = strtok ( t, delimiters );
	for ( i = 1; i < numtokens; i++ )
	{
	    *((*argvp) + i ) = strtok ( NULL, delimiters );
	}
    }
    *(( *argvp ) + numtokens ) = NULL;
    return ( numtokens );
}


void freemakeargv (
    INOUT char ** argv
    )
{
    if ( ! argv )
	return;

    if ( ! *argv )
	free ( * argv );

    free ( argv );
}


int main ( int argc, char ** argv )
{
    int     i;
    char    delim[8];
    char ** myargv;
    int     numtokens;
    int x = 42, y = 54;
    char line[80];
    sprintf ( line, "%d %d", x, y );

    strcpy ( delim, " \t" );
    if ( argc < 2 )
    {
        fprintf ( stderr, "Usage: %s string [delimiters]\n", argv[0] );
        return ( 1 );
    }

    if ( argc == 3 )
	strcpy ( delim, argv[2] );

    if ( ( numtokens = makeargv ( line, delim, &myargv ) ) == -1 )
    {
        fprintf ( stderr, "Failed to construct an argument array for %s\n", argv[1] );
        return ( 1 );
    }

    printf ( "The argument array contains:\n" );
    for ( i = 0; i < numtokens; i++ )
        printf ( "%d:%s\n", i, myargv[i] );

    return ( 0 );
}
