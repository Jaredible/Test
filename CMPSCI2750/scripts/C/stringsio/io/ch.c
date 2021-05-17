#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char ch = 'A';

    printf ( "%c = %o\n", ch, (int)ch );

    ch = 0102;

    printf ( "%c = %o\n", ch, (int)ch );

    char * str;
    str = ( char * ) malloc ( 4 * sizeof ( char ) );
    strcpy ( str, "Hello" );

    printf ( "String is: %s\n", str );

    return ( 0 );
}
