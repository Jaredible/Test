#include <stdio.h>

// Read a line of input from stdin and print in on stdout

int main()
{
    char str[80];

    printf ( "Please enter a message: " );
    gets ( str );

    printf ( "The message you entered is: " );
    puts ( str );

    return ( 0 );
}
