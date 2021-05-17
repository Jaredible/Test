#include <stdio.h>

int main() {

	char ch;

	for ( ch = 'a'; ch <= 'z'; ch++)
		printf( "%c", ch);

	printf("\n");

	int i;
	for (i = 0; i < 10; i++) {
		ch = getchar();
		printf("Character %d is:|%c|\n",i,ch);
	}

}
