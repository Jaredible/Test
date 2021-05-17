#include <stdio.h>
#include <stdlib.h>
//#include <string.h>

int main() {

	char s1[100];
	char s2[100];

	printf("Please enter string one:");

	fgets(s1,100,stdin);

	printf("Please enter string two:");	

	fgets(s2,100,stdin);

	if (strcmp(s1,s2) == 0) {
		printf("The strings are equal!\n");
	}
	else if (strcmp(s1,s2) < 0) {
		printf("s1 < s2\n");
	}
	else {
		printf("s1 > s2\n");
	}



	if (strcasecmp(s1,s2) == 0) {
		printf("The strings are equal ignoring case!\n");
	}
	else {
		printf("The strings are not equal ignoring case!\n");
	}


	if (strncasecmp(s1,s2,5) == 0) {
//	if (strcasecmp(s1,s2,5) == 0) {
		printf("The strings are equal to 5 characters ignoring case!\n");
	}
	else {
		printf("The strings are not equal to 5 characters ignoring case!\n");
	}

	return EXIT_SUCCESS;
}

