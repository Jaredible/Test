#include <stdio.h>

// Read a character from stdin and print in on stdout

int main()
{
    char ch;

    printf ( "Please enter a character: " );
    ch = getchar();

    printf ( "The character you entered is: " );
    putchar ( ch );
    putchar ( '\n' );

    return ( 0 );
}
