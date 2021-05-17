#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>
#include "defs.h"
#include "util.h"

int main ( int argc, char ** argv )
{
    key_t	key;
    int		shmid;
    pid_t	server, client;

    // Generate the key to access shared memory for the project

    if ( ( key = ftok ( KEY_FILE, PROJ_NUM ) ) < 0 )
    {
	die ( argv[0], "ftok" );
    }

    // Allocate shared memory

    if ( ( shmid = shmget ( key, SIZE, IPC_CREAT | 0666 ) ) < 0 )
    {
	die ( argv[0], "shmget" );
    }

    char parm[80];
    sprintf ( parm, "%d", shmid );

    // Create server and client

    if ( ( server = fork() ) < 0 )
    {
	die ( argv[0], "fork server" );
    }

    if ( ! server )		// Child process
    {
	execlp ( "./server", "server", parm, NULL );
	fprintf( stderr, "Error: Could not execute server\n" );
    }

    if ( ( client = fork() ) < 0 )
    {
	die ( argv[0], "fork child" );
    }

    if ( ! client )		// Child process
    {
	execlp ( "./client", "client", parm, NULL );
	fprintf( stderr, "Error: Could not execute client\n" );
    }

    int status;
    pid_t pid;
    pid = wait ( &status );
    if ( pid == server )
	fprintf ( stderr, "Server terminated\n");
    else if ( pid == client )
	fprintf ( stderr, "Client terminated\n");

    pid = wait ( &status );
    if ( pid == server )
	fprintf ( stderr, "Server terminated\n");
    else if ( pid == client )
	fprintf ( stderr, "Client terminated\n");

    // Release the allocated shared memory 

    if ( shmctl ( shmid, IPC_RMID, NULL ) < 0 )
	die( argv[0], "shmctl IPC_RMID");

    return ( 0 );
}
