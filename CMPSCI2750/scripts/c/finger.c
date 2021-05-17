/* From: rsalz@bbn.com (Rich Salz)
Subject: Re: how to put a program into a .plan file
Date: 27 Sep 90 19:13:51 GMT
*/
/*  $Revision$
**
**  A process to create dynamic .plan files.  Creates a fifo, waits for
**  someone to connect to it.  Optional first argument is the directory
**  to chdir(2) to, as in "plan ~rsalz &"; default is value of $HOME.
**
**  Right now, this just keeps a count and runs fortune.  A neat hack
**  would be to replace /usr/etc/in.fingerd with code that does a
**  getpeername()/gethostbyaddr(), and goes back to the requesting
**  host to do a finger there.
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#if	!defined(FD_SET)
    /* Some systems have it typedef'd wrong, so use #define. */
#define fd_set		int
#define	FD_SET(n, p)	(*(p) |= (1 << (n)))
#define	FD_CLR(n, p)	(*(p) &= ~(1 << (n)))
#define	FD_ISSET(n, p)	(*(p) & (1 << (n)))
#define	FD_ZERO(p)	(*(p) = 0)
#endif	/* !defined(FD_SET) */

extern int	errno;
extern char	*getenv();

static char	PLAN[] = ".plan";
static char	FORTUNE[] = "/usr/local/bin/cookie";


main(ac, av)
    int		ac;
    char	*av[];
{
    char	*p;
    int		Count;
    int		fd;
    FILE	*F;
    FILE	*In;
    fd_set	writers;
    char	buff[256];

    /* Go to the right directory. */
    if (ac == 2)
	p = av[1];
    else if ((p = getenv("HOME")) == NULL) {
	fprintf(stderr, "No $HOME.\n");
	exit(1);
    }
    if (chdir(p) < 0) {
	perror("Can't cd to $HOME");
	exit(1);
    }

    /* Remove any old one, create a new one. */
    if (unlink(PLAN) < 0 && errno != ENOENT) {
	perror("Can't unlink");
	exit(1);
    }
    (void)umask(0);
    if (mknod(PLAN, S_IFIFO | 0644, 0) < 0) {
	perror("Can't mknod");
	exit(1);
    }

    /* Enter the server loop. */
    for (Count = 0; ; ) {

	/* Open the fifo for writing. */
	if ((F = fopen(PLAN, "w")) == NULL) {
	    perror("Can't fopen");
	    (void)unlink(PLAN);
	    exit(1);
	}
	fd = fileno(F);

	/* Wait until someone else opens it for reading, so that we can
	 * write on it. */
	FD_ZERO(&writers);
	FD_SET(fd, &writers);
	if (select(fd + 1, (fd_set *)NULL, &writers, (fd_set *)NULL,
		(struct timeval *)NULL) < 0) {
	    if (errno != EINTR)
		perror("Can't select");
	    continue;
	}
	if (!FD_ISSET(fd, &writers))
	    /* "Can't happen" */
	    continue;

	/* Say hello to the nice stranger. */
	fprintf(F, "Hello there -- the plan is replaced by a fortune for you \n \n");
	if ((In = popen(FORTUNE, "r")) == NULL)
	    fprintf(F, "\tSorry, cookie jar is empty.\n");
	else {
	    while (fgets(buff, sizeof buff, In))
		fputs(buff, F);
	    (void)pclose(In);
	}

	/* Close it so they stop reading, and pause to make sure of it. */
	(void)fclose(F);
	(void)sleep(1);
    }

    /* NOTREACHED */
}
