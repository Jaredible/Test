#include <stdio.h>
#include <string.h>

// Accessing environment variables
// Looking for a specific variable

int main ( const int argc, char ** argv, char ** envp )
{
    char ** env_var;
    const char * var = "PATH";

    for ( env_var = envp; * env_var; env_var++ )
    {
	if ( ! strncmp ( var, *env_var, strlen ( var ) ) )
	    break;
    }

    if ( *env_var )
	printf ( "%s\n", *env_var );
    else
	printf ( "Variable %s not found\n", var );

    return ( 0 );
}
