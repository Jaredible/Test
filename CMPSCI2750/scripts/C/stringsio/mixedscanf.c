#include <stdio.h>

int main() {
	int i;
	float f;
	char str[100];
	char firststr[100];


	printf("Please enter a string (followed by enter), then an integer, a float and then a string\n");


	fgets(firststr,100,stdin);

	scanf("%d %f %s",&i,&f,str);

	printf("The integer entered was %d\n",i);

	printf("The float entered was %f\n",f);

	printf("The string entered was %s\n",str);

}
