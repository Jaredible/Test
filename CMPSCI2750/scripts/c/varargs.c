#include <stdio.h>
#include <stdarg.h>

int average ( int num_args, ... )
{
    int count = 0;
    int sum = 0;
    int i = 0, j;
    va_list	marker;

    va_start ( marker, num_args );
    for ( j = 0; j < num_args; j++ )
    {
	i = va_arg ( marker, int );
	sum += (int)i;
	count++;
    }

    va_end ( marker );
    printf ( "Sum: %d\n", sum );
    return ( count ? ( sum / count ) : 0 );
}

int main()
{
    int a = average ( 6, 3, 4, 5, 6, 7, 8 );
    int b = average ( 8, 1, 2, 3, 4, 5, 6, 7, 8 );
    int c = average ( 2, 42, 54 );
    printf ( "%d %d %d\n", a, b, c );

    return ( 0 );
}
