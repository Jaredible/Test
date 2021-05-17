//******************************************************************************
// acct.c: File to generate random account information for use in Cmp Sci 2750
//         assignment
// Author: Sanjiv K. Bhatia

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include "acct.h"

int main()
{
    FILE	* acct_file;	// Output file (binary)
    FILE	* infile;	// Input file containing names
    int		  errno;	// For system errors
    acct_info_t	  acct;		// Account information record
    char	* nl;		// Pointer to newline
    unsigned int  header[2];	// Magic number and number of records

    acct.number = 0;
    srand ( time ( NULL ) );

    acct_file = fopen ( "acct_info", "w" );
    if ( ! acct_file )
    {
	perror ( "Could not open the file acct_info" );
	return ( 1 );
    }

    infile = fopen ( "names", "r" );
    if ( ! infile )
    {
	perror ( "Could not open the file names" );
	return ( 1 );
    }

    header[1] = 0;
    while ( 1 )
    {
	fgets ( acct.name, sizeof ( acct.name ), infile );
	if ( feof ( infile ) )
	    break;
	header[1]++;
    }

    rewind ( infile );
    header[0] = ( toascii ( 'a' ) << 24 ) | ( toascii ( 'c' ) << 16 ) |
		( toascii ( 'c' ) << 8 ) | ( toascii ( 't' ) ) | 0x80808080;

    fwrite ( &header, sizeof ( int ), 2, acct_file );

    while ( 1 )
    {
	memset ( acct.name, '\0', sizeof ( acct.name ) );
	fgets ( acct.name, sizeof ( acct.name ), infile );
	if ( feof ( infile ) )
	    break;
	nl = strchr ( acct.name, '\n' );
	if ( nl )
	    *nl = '\0';
	acct.number++;
	acct.balance = (float)( rand() ) / RAND_MAX * 1000.0;

	fwrite ( &acct, sizeof ( acct_info_t ), 1, acct_file );
    }

    fclose ( acct_file );
    fclose ( infile );
    return ( 0 );
}
