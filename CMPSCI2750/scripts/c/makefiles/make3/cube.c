#include <stdio.h>

int square ( int );

int cube ( int x )
{
    printf ( "Modified cube\n" );
    return ( x * square ( x  ) );
}
