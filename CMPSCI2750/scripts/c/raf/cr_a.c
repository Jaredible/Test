/******************************************************************************/
/* Creating a randomly accessed file                                          */
/******************************************************************************/
#include <stdio.h>
#include "types.h"

int main()
{
    int      i;                      /* Loop counter                          */
    client_data_t blank_client = { 0, "", "", 0.00 };
    FILE   * client_file;

    if ( ( client_file = fopen ( "credit.asc", "w" ) ) == NULL )
    {
        printf ( "Could not open file credit.asc\n" );
        return ( 1 );
    }

    for ( i = 0; i < 100; i++ )
        fprintf ( client_file, "%3d%15s%15s%7.4f\n", blank_client.acct_num, \
	blank_client.last_name, blank_client.first_name, blank_client.balance );

    fclose ( client_file );

    return ( 0 );
}
/******************************************************************************/
