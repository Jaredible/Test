/******************************************************************************/
/* wr_fl.c : Capture user's inputs from the keyboard into a file              */
/******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <string.h>

int errno;

int main ( int argc, char ** argv )
{
    FILE *fd;                           /* File descriptor          */
    char line[100];                     /* Buffer for input         */
    char filename[] = "testfile";       /* File to keep information */

    // Prepare for errors

    char  err[80];
    strcpy ( err, "Error: " );
    strcat ( err, argv[0] );

    /* Open the file */

    if ( ! ( fd = fopen ( filename, "wx" ) ) )
    {
        fprintf ( stderr, "Sorry, could not not open file %s\n", filename );
	perror ( err );
        return ( 1 );
    }

    /* Get data from the user     */

    printf ( "Please enter the text to write to the file\n" );
    printf ( "Press ctrl-d on a line by itself when finished\n\n" );
    fgets ( line, sizeof ( line ), stdin );

    while ( ! feof ( stdin ) )	/* Check for end of file character */
    {
        fprintf ( fd, "%s", line );
        fgets ( line, sizeof ( line ), stdin );
    }

    fclose ( fd );

    return ( 0 );		/* Normal termination */
}
