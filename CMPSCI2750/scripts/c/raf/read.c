/******************************************************************************/
/* Reading data from a random access file                                     */
/******************************************************************************/
#include <stdio.h>
#include "types.h"

// #define DEBUG

int main()
{
    FILE   * client_file;
    client_data_t   client;

    if ( ( client_file = fopen ( "credit.dat", "r" ) ) == NULL )
    {
        printf ( "Could not open file credit.dat\n" );
        return ( 1 );
    }

    printf ( "%-6s %-15s %-15s %10s\n", "Acct", "Last name", "First name", "Balance" );

#ifdef DEBUG
    fprintf ( stderr, "Printed the header\n" );
#endif

    while ( ! feof ( client_file ) )
    {
        fread ( &client, sizeof ( client_data_t ), 1, client_file );
	if ( feof ( client_file ) )
	    break;
        if ( client.acct_num )
            printf ( "%-6d %-15s %-15s %10.2f\n", client.acct_num, \
                     client.last_name, client.first_name, client.balance );
    }

#ifdef DEBUG
    fprintf ( stderr, "Finished printing\n" );
#endif

    fclose ( client_file );

    return ( 0 );
}
/******************************************************************************/
