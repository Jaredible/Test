#include <stdio.h>

int main()
{
    char * color = "blue";
    char color_out[3];

    char * in = color;
    char * out = color_out;

    *out = *in;
    while ( *out != 0 )
    {
        out++;
        in++;
        *out = *in;
    }

    printf ( "In: %s   Out: %s\n", color, color_out );
    return ( 0 );
}

