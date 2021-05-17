#include <stdio.h>
#include <unistd.h>

int main() {
	printf("Before executing execl");
	//fflush(stdout);
	
	int e = execl("/bin/ls","ls","-al","*.c",NULL);

	if ( e )
		printf("\nError and didn't execute ls\n");

	printf("\nDone executing execln\n");

	return 0;
}
