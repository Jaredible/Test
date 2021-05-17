#include <stdio.h>
#include <stdlib.h>

//#define num 100000000000
#define num 10

struct Point {
	int x, y;
};


int main() {

	int * arr;

	// allocate an int array of size num
	arr = malloc(sizeof(int)*num);

	struct Point *p;

	p = malloc(sizeof(struct Point)*num);
	if (p == 0) {
		fprintf(stderr,"Could not allocate the memory!\n");
	}


	free(arr);
}
