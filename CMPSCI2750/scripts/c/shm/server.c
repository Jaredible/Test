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

    // Create messages

    char 	msgs[NUM_MSGS][80];
    strcpy ( msgs[0], "NASA sending robotic geologist to Mars to dig super deep" );
    strcpy ( msgs[1], "Jeff Bezos thinks his fortune is best spent in space" );
    strcpy ( msgs[2], "NASA cancels lunar rover, shifts focus to commercial moon landers" );
    strcpy ( msgs[3], "SpaceX Dragon to return 900 kg of experiments from ISS this weekend" );
    strcpy ( msgs[4], "Plan to bring back rocks from Mars is our best bet to find clues of past life" );

    shmid = atoi ( argv[1] );

    if ( ( shm = shmat( shmid, NULL, 0 ) ) == ( void * )( -1 ) )
    {
	die( argv[0], "shmat" );
    }

    memset ( shm, 0, SIZE );

    int i;
    for ( i = 0; i < NUM_MSGS; i++ )
    {
	while ( *shm )
	    sleep ( rand() % 5 );

	printf ( "Printing message %d at %s\n", i, print_time() );
	strcpy ( shm, msgs[i] );
    }

    // Wait for last message to be picked up

    while ( *shm )
	sleep ( 1 );

    if ( shmdt ( shm ) < 0 )
	die ( argv[0], "shmdt" );

    return ( 0 );
}
