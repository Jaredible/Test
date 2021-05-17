#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include "defs.h"
#include "util.h"

int main ( int argc, char ** argv )
{
    int		shmid;
    char *	shm;

    srand ( time ( NULL ) );

    shmid = atoi ( argv[1] );
    if ( ( shm = shmat( shmid, NULL, 0 ) ) == ( void * )( -1 ) )
    {
	die( argv[0], "shmat" );
    }

    int i;
    for ( i = 0; i < NUM_MSGS; i++ )
    {
	while ( ! *shm )
	    sleep ( rand() % 5 );

	printf ( "Retrieving message %d at %s\n", i, print_time() );
	printf ( "Msg %d:%s\n", i, shm );
	shm[0] = '\0';
    }

    if ( shmdt ( shm ) < 0 )
	die ( argv[0], "shmdt" );

    return ( 0 );
}
