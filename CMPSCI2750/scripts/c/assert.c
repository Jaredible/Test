#include <stdio.h>
#include <assert.h>

int main()
{
    int	num;
    printf ( "We are going to play a game where I'll think of a number\n" );
    printf ( "This number is between 1 and 1000\n" );
    printf ( "Your job is to guess it\n\n" );

    printf ( "Make a guess: " );
    scanf ( "%d", &num );

    assert ( num > 0 && num <= 1000 );

    printf ( "That is a valid guess\n" );

    return ( 0 );
}
