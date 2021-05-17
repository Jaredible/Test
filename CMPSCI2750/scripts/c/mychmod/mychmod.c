#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define	IN
#define	OUT
#define	INOUT

int errno;	// Required with global scope due to the use of perror()

//******************************************************************************
// Print usage message
// Should be called with argv[0] to print the program name in help

void showhelp (
    IN		char * prog_name		// prog_name is argv[0]
    )
{
    fprintf ( stderr, "usage: %s -u rwx -g rwx -o rwx -U rwx -G rwx -O rwx file [file ...]\n", \
    		prog_name );
    fprintf ( stderr, "-u [rwx]   User permissions to be added\n" );
    fprintf ( stderr, "-g [rwx]   Group permissions to be added\n" );
    fprintf ( stderr, "-o [rwx]   Others permissions to be added\n" );
    fprintf ( stderr, "-U [rwx]   User permissions to be removed\n" );
    fprintf ( stderr, "-G [rwx]   Group permissions to be removed\n" );
    fprintf ( stderr, "-O [rwx]   Others permissions to be removed\n" );
}

//******************************************************************************
// Make sure that the option arguments are limited to rwx
// Returns the offending argument, if it finds one
// Returns 0 on complete validation

char validate_opts (
    IN		char	* const	args
    )
{
    char * argp;
    for ( argp = args; argp; argp++ )
        if ( ! ( *argp == 'r' || *argp == 'w' || *argp == 'x' ) )
	    return ( *argp );

    return ( '\0' );
}

//******************************************************************************
// Parse the arguments
// Returns the arguments to be added and subtracted in the form of a mode_t
//	structure

int parser (
    IN		int	   argc,
    IN		char	** argv,
    OUT		mode_t	*  addperm,		// Permissions to be added
    OUT 	mode_t	*  rmperm		// Permissions to be removed
    )
{
    char	  opt;
    char	* args;

    // Make the structures to add and remove permissions to 0

    *addperm ^= *addperm;
    *rmperm  ^= *rmperm;

    // Get all permissions

    while ( ( opt = getopt ( argc, argv, "u:g:o:U:G:O:h" ) ) != -1 )
    {
	char ch;
	switch ( opt )
        {
	case 'u':
	    args = optarg;
	    strstr ( args, "r" ) && ( *addperm |= S_IRUSR );
	    strstr ( args, "w" ) && ( *addperm |= S_IWUSR );
	    strstr ( args, "x" ) && ( *addperm |= S_IXUSR );
	    break;

	case 'g':
	    args = optarg;
	    strstr ( args, "r" ) && ( *addperm |= S_IRGRP );
	    strstr ( args, "w" ) && ( *addperm |= S_IWGRP );
	    strstr ( args, "x" ) && ( *addperm |= S_IXGRP );
	    break;

	case 'o':
	    args = optarg;
	    strstr ( args, "r" ) && ( *addperm |= S_IROTH );
	    strstr ( args, "w" ) && ( *addperm |= S_IWOTH );
	    strstr ( args, "x" ) && ( *addperm |= S_IXOTH );
	    break;

	case 'U':
	    args = optarg;
	    strstr ( args, "r" ) && ( *rmperm |= S_IRUSR );
	    strstr ( args, "w" ) && ( *rmperm |= S_IWUSR );
	    strstr ( args, "x" ) && ( *rmperm |= S_IXUSR );
	    break;

	case 'G':
	    args = optarg;
	    strstr ( args, "r" ) && ( *rmperm |= S_IRGRP );
	    strstr ( args, "w" ) && ( *rmperm |= S_IWGRP );
	    strstr ( args, "x" ) && ( *rmperm |= S_IXGRP );
	    break;

	case 'O':
	    args = optarg;
	    strstr ( args, "r" ) && ( *rmperm |= S_IROTH );
	    strstr ( args, "w" ) && ( *rmperm |= S_IWOTH );
	    strstr ( args, "x" ) && ( *rmperm |= S_IXOTH );
	    break;

	case 'h':
	    showhelp ( argv[0] );
	    return ( 1 );

	default:
	    fprintf ( stderr, "Fatal: Unknown option\n" );
	    return ( 1 );
	}

	if ( ( ch = validate_opts ( args ) ) )
	    fprintf ( stderr, "Warning: Unrecognized argument to option %c: %c\n", opt, ch );
    }

	if ( *addperm & *rmperm )
	    fprintf ( stderr, "Warning: Trying to add and subtract same permissions\n" );

    return ( 0 );
}


//******************************************************************************

int main ( int argc, char ** argv )
{
    char * prog_name = argv[0];
    mode_t	addperm;
    mode_t	rmperm;
    mode_t	newperms;

    // Get the options

    if ( parser ( argc, argv, &addperm, &rmperm ) || optind == argc )
    {
        showhelp ( prog_name );
	return ( 1 );
    }

    // Modify the permissions for each file

    for ( ; optind < argc; optind++ )
    {
	struct stat	  stat_buf;		// To retrieve file information
	mode_t		  perms;		// Current permissions on file
	uid_t		  f_uid;		// User owner of file
        char		* fn = argv[optind];

	// Get the current permissions for the file

	if ( stat ( fn, &stat_buf ) < 0 )
	{
	    fprintf ( stderr, "Cannot access file %s\n", fn );
	    continue;
	}

	perms = stat_buf.st_mode;	// Current permissions on file
	f_uid = stat_buf.st_uid;	// User owner of file
	if ( f_uid != geteuid() )
	{
	    fprintf ( stderr, "You are not owner of file %s\n", fn );
	    continue;
	}

	newperms = perms | addperm;
	newperms = newperms & ( ~ rmperm );
    	if ( chmod ( fn, newperms ) < 0 )
	{
	    char errmsg[80];
	    sprintf ( errmsg, "%s: Some problem changing permissions on file %s", argv[0], fn);
	    perror ( errmsg );
    	}
    }

    return ( 0 );
}
