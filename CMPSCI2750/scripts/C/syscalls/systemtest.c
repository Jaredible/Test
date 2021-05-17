#include <stdio.h>
#include <stdlib.h>

int main() {
	char buf[80];

	printf("Before making call to system()...\n");

	sprintf(buf,"date -u");

	system(buf);

	printf("Did it work?\n");

	sleep(4);

	printf("Looks like it did!\n");

	return 0;
}
