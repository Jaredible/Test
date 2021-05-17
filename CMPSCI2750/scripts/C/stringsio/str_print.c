#include <stdio.h>

int main()
{
    char str[] = "Hello world";
    char line[80];
    printf ( "First five characters are: |%.5s|\n", str );
    printf ( "Trying to print in five character field gives: |%5s|\n", str );
    printf ( "Printing in 20 character field gives: |%20s|\n", str );
    printf ( "Left justification is achieved by: |%-20s|\n", str );

    puts ( "Illustrating puts: " );
    puts ( str );

    sprintf ( line, "Showing the use of sprintf: %s", str );
    puts ( line );

    return ( 0 );
}
