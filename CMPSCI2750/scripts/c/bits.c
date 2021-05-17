#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * display ( unsigned char * x )
{
    int i;
    char * byte = malloc ( 9 * sizeof ( char ) );
    if ( ! byte )
    {
	perror ( "Error: " );
	exit ( 1 );
    }
    memset ( byte, '\0', sizeof ( byte ) );
    for ( i = 7; i >= 0; i-- )
	byte[7-i] = ( *x & ( 1 << i ) ) ? '1' : '0';
    return ( byte );
}

int main()
{
    char x = 64;
    char y = -128;
    unsigned char z = 128;

    printf ( "Original Values:\n" );
    printf ( "x = %3d %s  y = %3d %s  z = %3d %s\n", x, display ( &x ), y, display ( &y ), z, display ( &z ) );

    x <<= 1;
    y <<= 1;
    z <<= 1;

    printf ( "After multiplication by 2:\n" );
    printf ( "x = %3d %s  y = %3d %s  z = %3d %s\n", x, display ( &x ), y, display ( &y ), z, display ( &z ) );

    x >>= 1;
    y >>= 1;
    z >>= 1;

    printf ( "After dividing by 2:\n" );
    printf ( "x = %3d %s  y = %3d %s  z = %3d %s\n", x, display ( &x ), y, display ( &y ), z, display ( &z ) );

    x = 64;
    y = -128;
    z = 128;

    x >>= 1;
    y >>= 1;
    z >>= 1;

    printf ( "After dividing originals by 2:\n" );
    printf ( "x = %3d %s  y = %3d %s  z = %3d %s\n", x, display ( &x ), y, display ( &y ), z, display ( &z ) );

    return ( 0 );
}
