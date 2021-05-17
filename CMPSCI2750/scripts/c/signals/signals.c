// Program to illustrate the blocking of SIGINT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>


int main ( int argc, char ** argv )
{
    int			i;
    sigset_t		intmask;
    int			repeat_factor;
    double		y = 0.0;

    if ( argc != 2 )
    {
	fprintf ( stderr, "usage: %s repeatfactor\n", argv[0] );
	return ( 1 );
    }

    repeat_factor = atoi ( argv[1] );

    // sigemptyset initialializes the set intmask to exclude all signals
    // defined by the system; sigaddset adds SIGINT to the set intmask

    if ( ( sigemptyset ( &intmask ) == -1 ) ||
	 ( sigaddset ( &intmask, SIGINT ) == -1 ) )
    {
	perror ( "Failed to initialize the signal mask" );
	return ( 1 );
    }

    for ( ; ; )
    {
	// Add intmask to the current signal mask

	if ( sigprocmask ( SIG_BLOCK, &intmask, NULL ) == -1 )
	    break;

	fprintf ( stderr, "SIGINT signal blocked\n" );
	sleep ( 10 );
	for ( i = 0; i < repeat_factor; i++ )
	    y += sin ( ( double ) i );
	fprintf ( stderr, "Blocked calculation is finished, y = %f\n", y );

	// Remove the set intmask from current signal mask

	if ( sigprocmask ( SIG_UNBLOCK, &intmask, NULL ) == -1 )
	    break;

	fprintf ( stderr, "Signal is now unblocked\n" );
	sleep ( 10 );
	for ( i = 0; i < repeat_factor; i++ )
	    y += sin ( ( double ) i );
	fprintf ( stderr, "Unblocked calculation is finished, y = %f\n", y );
    }

    perror ( "Failed to change signal mask\n" );

    return ( 1 );
}
