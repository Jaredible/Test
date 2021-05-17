#include <stdio.h>

int main() {
	int i;
	float f;
	char str[100];


	printf("Please enter an integer, a float and then a string\n");

	scanf("%d %f %s",&i,&f,str);

	printf("The integer entered was %d\n",i);

	printf("The float entered was %f\n",f);

	printf("The string entered was %s\n",str);

}
