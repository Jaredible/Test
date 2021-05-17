#include <stdio.h>
#include <unistd.h>

int main() {
	int pid;
	int j=0;
	printf("Ready to fork...\n");

	pid = fork();

	if (pid == 0) {
		printf("Child %d is executing some code here\n",getpid());
		int i = 0;
		for (i = 0; i < 5; i++) {
			j+= i;
			sleep(1);
			printf("Child %d has a j=%d\n",getpid(),j);
		}
	}
	else {
		int statuscode;
		j = wait(&statuscode);
		printf("Should only see this after child is done!\n");
		printf("The child returned a status code of %d\n",statuscode);
		printf("Parent has a j=%d\n",j);

	}

	if (pid ==0)
		return -1;
	else
		return 0;
}
