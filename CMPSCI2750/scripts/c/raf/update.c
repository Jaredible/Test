/******************************************************************************/
/* Updating a randomly accessed file                                          */
/******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include "types.h"

int main()
{
    FILE   * client_file;
    client_data_t   client;
    char     line[80];                 /* Input buffer for stdin              */

    if ( ( client_file = fopen ( "credit.dat", "r+" ) ) == NULL )
    {
        printf ( "Could not open file credit.dat\n" );
        return ( 1 );
    }

    printf ( "Enter account number (valid range: 1 -- 100; 0 to quit) : " );
    fgets ( line, sizeof(line), stdin );
    sscanf ( line, "%d", &client.acct_num );

    while ( client.acct_num )
    {
        printf ( "Enter last name, first name, and balance : " );
        fgets ( line, sizeof(line), stdin );
        sscanf (line, "%s%s%f", &client.last_name, &client.first_name, &client.balance);

        fseek (client_file, (client.acct_num-1)*sizeof(client_data_t), SEEK_SET);
        fwrite (&client, sizeof(client_data_t), 1, client_file);

        printf ( "Enter account number (valid range: 1 -- 100; 0 to quit) : " );
        fgets ( line, sizeof(line), stdin );
        sscanf ( line, "%d", &client.acct_num );
    }

    fclose ( client_file );

    return ( 0 );
}
/******************************************************************************/
