/******************************************************************************/
/* char_fns.c : Program to illustrate different character functions           */
/* Author: Sanjiv K. Bhatia                                                   */
/* Date  : March 13, 2012                                                     */
/******************************************************************************/

#include <stdio.h>

int main()
{
    int foo;

    printf ( "Please type in a hex integer: " );
    scanf ( "%x", &foo );

    printf ( "The decimal value of %x is %d\n", foo, foo );

    if ( isalpha ( foo ) )
    {
        printf ( "%x is a letter\n", foo );
        printf ( "The character equivalent of %x is %c\n", foo, foo );
        printf ( "%x is a %s letter\n", \
                 foo, isupper(foo) ? "uppercase" : "lowercase" );
    }

    if ( isdigit ( foo ) )
        printf ( "%x is a digit\n", foo );

    if ( isxdigit ( foo ) )
        printf ( "%x is a hexadecimal digit\n", foo );

    if ( isalnum ( foo ) )
        printf ( "%x is an alphanumeric character\n", foo );

    if ( isspace ( foo ) )
        printf ( "%x is a whitespace character\n", foo );

    if ( ispunct ( foo ) )
        printf ( "%x is a punctuation character\n", foo );

    if ( iscntrl ( foo ) )
        printf ( "%x is a control character\n", foo );

    if ( isascii ( foo ) )
        printf ( "%x is an ASCII character %c\n", foo, foo );
    else
    {
        printf ( "The ASCII equivalent of %x is %x", foo, toascii(foo) );
        printf ( isprint(toascii(foo)) ? ", or character %c\n" : "\n", foo );
    }

    if ( isgraph ( foo ) )
        printf ( "%x is a visible graphic character %c\n", foo, foo );

    return ( 0 );
}
