// Echos the command line arguments given it
// to stdout

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

	// Allow exactly 3 arguments
	if (argc != 4) {
		printf("Please call me with 3 args!\n");
		return EXIT_FAILURE;
	}

	int i = 0;

	printf("Our command line args are:\n");
	while ( i < argc) {
		printf("%s\n", argv[i++]);
	}

	return EXIT_SUCCESS;
}
