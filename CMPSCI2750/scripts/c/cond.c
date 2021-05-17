#include <stdio.h>

int main()
{
    printf ( "Hello world\n" );

#ifdef VAL
    printf ( "%d\n", VAL );
#endif

#ifdef DEBUG
    printf ( "Hello debugger world\n" );
#else
    printf ( "Hello release world\n" );
#endif

    return ( 0 );
}
