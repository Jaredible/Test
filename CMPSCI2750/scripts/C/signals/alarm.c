// Program to illustrate default action for alarm system call
// alarm(2) by default terminates the process that executes it after specified
// number of seconds

#include <stdio.h>
#include <unistd.h>

int main( const int argc, char ** argv )
{
    int i;
    unsigned int interval = 10;		// Default interval

    if ( argc == 2)
    {
	interval = atoi ( argv[1] );	// Override default
    }

    alarm ( interval );		// Terminate after interval seconds

    // Start an infinite loop

    for ( i = 0; ; i++ )
    {
	printf ( "%d\n", i );
	sleep ( 1 );
    }
}
