#include <stdio.h>

/* Prototypes */

int square ( int );
int cube (int );

int main ( void )
{
    int n, s, c;
    printf ( "Please enter a number: " );
    scanf ( "%d", &n );

    s = square ( n );
    c = cube ( n );

    printf ( "Square of %d is %d\n", n, s );
    printf ( "Cube of %d is %d\n", n, c );

    return ( 0 );
}
