#include <stdio.h>
#include <unistd.h>

int main() {

	printf("Ready to fork...\n");
	int pid = fork();

	if (pid == -1) {
		printf("Failed to fork, perhaps try to spoon?\n");
	}

	if (pid == 0) {
		printf("Im the child process with a pid of %d, my parents pid is %d!\n"
			,getpid(),getppid());
			
	}
	else {
		printf("Im the parent process, my child has a PID of %d, my own pid is %d\n",
			pid,getpid()); 
	}
	
	return 0;
}
