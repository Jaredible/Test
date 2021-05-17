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

    if ( ( client_file = fopen ( "credit.dat", "w" ) ) == NULL )
    {
        printf ( "Could not open file credit.dat\n" );
        return ( 1 );
    }

    for ( i = 0; i < 100; i++ )
        fwrite ( &blank_client, sizeof ( client_data_t ), 1, client_file );

    fclose ( client_file );

    return ( 0 );
}
/******************************************************************************/
