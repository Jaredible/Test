#include <stdio.h>
#include <stdlib.h>

#define num 10

typedef struct Point {
	int x, y;
} Point;


int main() {

	int * arr;

	// allocate an int array of size num
	arr = malloc(sizeof(int)*num);

	Point *p;

	p = malloc(sizeof(Point)*num);


	free(arr);
	free(p);
}
