// Echos the command line arguments given it
// to stdout

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

	int i = 0;

	while ( i < argc) {
		printf("%s\n", argv[i++]);
	}

	printf("\n");
	return EXIT_SUCCESS;
}
