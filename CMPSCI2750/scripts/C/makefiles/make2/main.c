#include <stdio.h>

/* Prototypes */

int square ( int );
int cube (int );
int quad ( int );

int main ( void )
{
    int n, s, c, q;
    printf ( "Please enter a num: " );
    scanf ( "%d", &n );

    s = square ( n );
    c = cube ( n );
    q = quad ( n );

    printf ( "Square of %d is %d\n", n, s );
    printf ( "Cube of %d is %d\n", n, c );
    printf ( "Quad of %d is %d\n", n, q );

    return ( 0 );
}
