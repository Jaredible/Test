#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


int main(int argc, char *argv[])
{
    int flags, opt;
    int nsecs, tfnd;

    nsecs = 0;
    tfnd = 0;
    flags = 0;
    
    while ((opt = getopt(argc, argv, "hvnt:")) != -1)
    {
	switch (opt)
	{
            case 'h':
		fprintf(stdout, "Usage: %s [-t nsecs] [-n] name\n", argv[0]);
		exit(EXIT_FAILURE);
	    case 'n':
		flags = 1;
		break;
	    case 'v':
		/// blah blah blah
		break;
	    case 't':
		nsecs = atoi(optarg);
		tfnd = 1;
		break;

	    default: /* '?' */
		fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    }

    printf("flags=%d; tfnd=%d; optind=%d; argc=%d\n", flags, tfnd, optind,argc);

    // Options handled; start looking for arguments
    //
    if (optind >= argc)
    {
	fprintf(stderr, "Expected argument after options\n");
	exit(EXIT_FAILURE);
    }

    printf("name argument = %s\n", argv[optind]);

    /* Other code omitted */

    exit(EXIT_SUCCESS);
}
