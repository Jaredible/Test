#include <stdio.h>

void spawnProducer();
void spawnConsumer();

void main() {
	spawnProducer();
	spawnConsumer();
	
	while (wait(NULL) > 0);
}

void spawnProducer() {
	printf("Spawning producer\n");
	
	int pid = fork();
	
	if (pid == -1) {
		perror("fork");
		exit(1);
	}
	
	if (pid == 0) {
		execl("./consumer", "consumer");
		perror("execl");
		exit(0);
	}
}

void spawnConsumer() {
	printf("Spawning consumer\n");
}
