#include <stdio.h>
#include <stdlib.h>

int main( int argc, char ** argv )
{
    int r;
    int num = 1;
    int i = 0;

    if ( argc == 2 )
	num = atoi ( argv[1] );

    srand ( time ( NULL ) );

    for ( ; i < num; i++ )
    {
	r = rand() % 100;
	printf ( "%2d %6d\n", i, r );
    }

    return ( 0 );
}
