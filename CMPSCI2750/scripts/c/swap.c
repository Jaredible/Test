#include <stdio.h>
#include <assert.h>

void * tmp;

#define swap(x,y) \
    assert ( sizeof ( x ) == sizeof ( y ) ); \
    tmp = malloc ( sizeof ( x ) ); \
    memcpy ( tmp, &x, sizeof ( x ) ); \
    memcpy ( &x, &y, sizeof ( x ) ); \
    memcpy ( &y, tmp, sizeof ( x ) ); \
    free ( tmp )

int main()
{
    int a = 42;
    int b = 54;

    double c;

    printf ( "%d %d\n", a, b );

    swap ( a, b );

    printf ( "%d %d\n", a, b );

    swap ( a, c );

    return ( 0 ); 
}
