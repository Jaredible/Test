#include <stdio.h>

int little_endian ()
{
    int i = 1;
    char c = *( char * )( &i );
    
    return ( c );
}

int main()
{
    printf ( "This machine is %s endian\n", little_endian() ? "little" : "big" );

    return ( 0 );
}
