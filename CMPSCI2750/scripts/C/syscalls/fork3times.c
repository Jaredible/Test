#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

int n = 3;

pid_t pid;
int i;
for ( i = 0; i < n; i++ )
{
    pid = fork();
    if ( pid < 0 )
    {
        perror ( "Error in fork: " );
        exit ( 1 );
    }

    if ( pid == 0 )
    {
        break;
    }
}

if ( pid == 0 )
{
    // Do whatever the child does
	printf("In child\n");
}
else  // You are in parent
{

    for ( i = 0; i < 3; i++ )
    {
	int status;
	// Wait for all children to end
        pid_t pid_ret = wait ( &status );

	printf("Child %d has terminated!\n",pid_ret);
    }
}

	return 0;
}
