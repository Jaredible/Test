#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void handle_fpe(int x);

void handle_fpe(int x) {
	printf("Dividing by 0 is derpity derp!\n");
	exit(SIGFPE);
}


int main() {

	int i;

	signal(SIGFPE,handle_fpe);

	for (i = -3; i <= 3; i++) {
		printf("%d\n",12/i);
	}

	return 0;
}

