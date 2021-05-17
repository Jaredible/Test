#include <stdio.h>

int main()
{
    int a[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int six = 6;

    printf ( "a[6] = %d\n", a[6] );
    printf ( "6[a] = %d\n", 6[a] );
    printf ( "*(a+6) = %d\n", *(a+six) );

    return ( 0 );
}
