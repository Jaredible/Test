#include <stdio.h>
#include <string.h>

// Accessing environment variables
// Looking for a specific variable
// Accessing individual directories

int main ( const int argc, char ** argv, char ** envp )
{
    char ** env_var;
    const char * var = "PATH";

    for ( env_var = envp; * env_var; env_var++ )
    {
	if ( ! strncmp ( var, *env_var, strlen ( var ) ) )
	    break;
    }

    if ( ! *env_var )
    {
	printf ( "Variable %s not found\n", var );
	return ( 1 );
    }

    printf ( "%s\n", *env_var );
    char * path = strtok ( *env_var, "=" );
    path = strtok ( NULL, "=" );

    char * dir;
    
    for ( dir = strtok ( path, ":" ); dir; dir = strtok ( NULL, ":" ) )
	printf ( "%s\n", dir );

    return ( 0 );
}
