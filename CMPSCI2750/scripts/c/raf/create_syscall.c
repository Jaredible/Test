/******************************************************************************/
/* Creating a randomly accessed file                                          */
/******************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "types.h"

int main()
{
    int      i;                      /* Loop counter                          */
    client_data_t blank_client = { 0, "", "", 0.00 };
    int      client_file;
    char     filename[] = "credit.dat";

    if ( ( client_file = open ( filename, O_WRONLY | O_CREAT ) ) == -1 )
    {
        printf ( "Could not open file %s\n", filename );
        return ( 1 );
    }

    for ( i = 0; i < 100; i++ )
	write ( client_file, &blank_client, sizeof ( client_data_t ) );

    close ( client_file );

    return ( 0 );
}
/******************************************************************************/
