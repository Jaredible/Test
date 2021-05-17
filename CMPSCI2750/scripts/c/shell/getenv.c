#include <stdio.h>

int main()
{
    char * env;

    env = getenv ( "MYPATH" );
    if ( env == NULL )
    {
	printf ( "No MYPATH in environment\n" );
	env = getenv ( "PATH" );
    }

    printf ( "Environment: %s\n", env );

    return ( 0 );
}
