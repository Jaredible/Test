#include <stdio.h>
#include <stdlib.h>

int main ( const int argc, const char ** argv )
{
    if ( argc < 2 )
    {
	fprintf ( stderr, "usage: %s number\n", argv[0] );
	return ( 1 );
    }

    int num = atoi ( argv[1]);
    printf ( "Argument is: %d\n", num );

    char str[] = {'a', 'b', 'c', '\0'};
    printf ( "String is: %s\n", str );
    return ( 0 );
}
