#include <stdio.h>
#include "functions.h"

int main() {
	printf("Please enter a number:");
	
	int num = 0;

	scanf("%d",&num);

	if ( calceven(num)== 0) {
		printf("The number is even isnt that awesome!\n");
	}
	else {

		printf("The number is odd!\n");
	}
}
