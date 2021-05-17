#include <stdio.h>

int main() {

	float f = 1237.672;
	printf("The float f is %f\n",f);

	// Integer inserted between percent and conversion specifier
	// indicates width of field

	printf("10,2 is: %10.2f\n",f);
	printf("8,2 is: %8.2f\n",f);
	printf("5,2 is: %5.2f\n",f);
	printf("3,2 is: %3.2f\n",f);
	printf("2,2 is: %2.2f\n",f);
	printf("1,2 is: %1.2f\n",f);


	printf("5,2 is: %5.2f\n",f);
	printf("5,3 is: %5.3f\n",f);
	printf("5,4 is: %5.4f\n",f);
	printf("5,5 is: %5.5f\n",f);
	printf("5,6 is: %5.6f\n",f);



}
