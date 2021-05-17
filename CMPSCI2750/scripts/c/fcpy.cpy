#include <stdio.h>
#include <string.h>

int main()
{
    FILE *infile, *outfile;	/* File descriptor */
    char line[100];
    char file_1[40], file_2[40];
    int n;

    printf ( "What is the source file? " );
    fgets ( file_1, sizeof ( file_1 ), stdin );
    printf ( "What is the destination file? " );
    fgets ( file_2, sizeof ( file_2 ), stdin );

    n = strlen ( file_1 );
    file_1[n-1] = NULL;
    n = strlen ( file_2 );
    file_2[n-1] = NULL;

    /* Open the files */

    if ( ! ( infile = fopen ( file_1, "r" ) ) )
    {
	printf ( "Sorry, could not open file %s for reading\n", file_1 );
	exit ( 1 );
    }

    if ( ! ( outfile = fopen ( file_2, "w" ) ) )
    {
	printf ( "Sorry, could not not open file %s to write\n", file_2 );
	exit ( 2 );
    }

    fgets ( line, sizeof ( line ), infile );

    while ( line[0] != '.' )
    {
	if ( feof ( infile ) )
	    break;
	fprintf ( outfile, "%s", line );
	fgets ( line, sizeof ( line ), infile );
    }

    /* Close the files */

    fclose ( infile );
    fclose ( outfile );

    return ( 0 );		/* Normal termination */
}
