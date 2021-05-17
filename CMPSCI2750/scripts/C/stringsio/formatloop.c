#include <stdio.h>

int main() {

	float f = 1237.672;
	printf("The float f is %f\n",f);

	// Integer inserted between percent and conversion specifier
	// indicates width of field

	printf("10,2 is: %10.2f\n",f);

	int w = 1;
	int p = 1; 
	for (w = 1; w < 10; w = w + 2) {
		for (p = 1; p < 6; p = p + 2) {
			printf("%d.%d is : %*.*f\n",w,p,w,p,f);
		} 

	}
}
