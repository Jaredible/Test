#include <stdio.h>

int odd ( int x )	//Tell me if x is odd
{
    return ( x & 1 );
}

int main()
{
    int num;
    printf ( "Please enter a number: " );
    scanf ( "%d", &num );

    printf ( "%d is %s\n", num, odd ( num ) ? "odd" : "even" );

    unsigned char inv = ~((unsigned char)(num));
    printf ( "%x's inverse is: %x\n", num, inv );

    printf ( "%x'slast four bits are: %x\n", num, num & 0x0F );

    return ( 0 );
}
