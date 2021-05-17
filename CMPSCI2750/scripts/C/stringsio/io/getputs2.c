#include <stdio.h>
#include <string.h>

// Read a line of input from stdin and print in on stdout

int main()
{
    const int sz = 80;		//Size of input line
    char str[sz];

    printf ( "Please enter a message: " );
    fgets ( str, sz, stdin );
    str[strlen(str)-1] = '\0';

    printf ( "The message you entered is: " );
    puts ( str );

    return ( 0 );
}
