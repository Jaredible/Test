#include <stdio.h>
#include "types.h"
#define extern
#include "extern.h"
#undef extern

#define LEN 100

/* Program to illustrate preprocessor */

int main()
{
    int x = 5;

    printf ( "Card type size is %ld\n", sizeof ( card_t ) );
    printf ( "%d\n", LEN * LEN );

    printf ( "Square of %d is %d\n", x, square ( x ) );
}
