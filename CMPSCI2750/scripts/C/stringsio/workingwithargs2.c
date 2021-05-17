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

	int i = 1;

	printf("Our command line args are:\n");
	while ( i < argc) {
		printf("%s\n", argv[i++]);
	}

	int num = atoi(argv[1]);

	float f = atof(argv[2]);

	long lnum = atol(argv[3]);

	printf("Our numbers are:\n");
	printf("\tinteger of %d\n",num);	
	printf("\tfloat of %f\n",f);	
	printf("\tlong integer of %d\n",lnum);	
	return EXIT_SUCCESS;
}
