#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

long int file_sz ( char * file_name )
{
    // Get information about file

    struct stat buf;
    if ( stat ( file_name, &buf ) < 0 )
    {
	perror ( "Error: " );
	return ( -1 );
    }

    // Return the size of file

    return ( buf.st_size );
}

int main( int argc, char ** argv )
{
    if ( argc < 2 )
    {
	fprintf ( stderr, "usage: %s filename\n", argv[0] );
    }

    char * file_name = argv[1];
    long int ret = file_sz ( file_name );
    if ( ret != -1 )
    {
	printf ( "Size of file %s is %ld bytes\n", file_name, ret );
	return ( 0 );
    }
}
