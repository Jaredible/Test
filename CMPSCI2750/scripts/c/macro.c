#include <stdio.h>

#define max(x,y) ((x) > (y) ? (x) : (y))
// #define max(x,y) (x > y ? x : y)
// #define _DEBUG

int main()
{
    int i, j;
    float a, b;
    printf ( "Enter two integers and two real numbers: " );
    scanf ( "%d %d %f %f", &i, &j, &a, &b );
    printf ( "Maximum values: %d %f\n", max(i,j), max(a,b) );

#if 0
    a = 1 + max ( b = c + 2, d );
#endif

#ifdef _DEBUG
    printf ( "Max incremented: %d %d %d\n", i, j, max ( i++, j++ ) );
#endif

    return ( 0 );
}
