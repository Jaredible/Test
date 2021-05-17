#include <stdio.h>

#define output(parm) printf(titleStr,#parm); printf("%5d\n",a.parm)

int main ( int argc, char ** argv )
{
    typedef struct
    {
    	int		x;
    	int		y;
    } obj;

    char *titleStr = "%10s";
    obj a = { 4, 2};

    output(x);
    output(y);

    return ( 0 );
}

