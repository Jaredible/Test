/******************************************************************************/
/* Printing an unsigned integer in bits                                       */

#include    <stdio.h>

void display_bits ( unsigned int );

int main()
{
    unsigned int x;                          /* Number to be printed in bits  */

    printf ( "Enter an unsigned integer: " );
    scanf ( "%u", &x );
    display_bits ( x );
}

void display_bits ( unsigned int value )
{
    unsigned int i,
                 display_mask = 1 << ( 8 * sizeof ( unsigned int ) - 1 );

    /* The display_mask contains 1 shifted by the number of bits in int       */

    printf ( "%7u = ", value );

    for ( i = 1; i <= ( 8 * sizeof ( unsigned int ) ); i++ )
    {
        putchar ( ( value & display_mask ) ? '1' : '0' );
        value <<= 1;

        if ( !( i % 8 ) )
            putchar ( ' ' );
    }

    putchar ( '\n' );
}
