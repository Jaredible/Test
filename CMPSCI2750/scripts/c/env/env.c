#include <stdio.h>

// Accessing environment variables

int main ( const int argc, char ** argv, char ** envp )
{
    char ** env_var;
    for ( env_var = envp; *env_var; env_var++ )
	printf ( "%s\n", *env_var );

    return ( 0 );
}
