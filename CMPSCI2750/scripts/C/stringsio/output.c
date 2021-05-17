#include <stdio.h>

int main() {

	int d = 57463;
	printf("The number d is %d\n",d);

	float f = 37.672;
	printf("The float f is %f\n");


	printf("Both numbers are %d %f\n",d,f);

	char format_str[] = "Our very own format string shows f is %f\n";

	printf(format_str,f);

	printf("This time our format string is just a string:\n %s\n",format_str);		

	return 0;
}
