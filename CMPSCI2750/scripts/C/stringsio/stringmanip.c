#include <stdio.h>
#include <ctype.h>


// for the string functions strncpy, etc
#include <string.h>

int main() {

	// These are two different things 
	char color[] = "blue";
	char * color_ptr = "blue";

	printf("Our first string is %s\n",color);
	printf("Our pointer to another string is %s\n",color_ptr);

	// can modify color
	color[0] = 'r';
	color[1] = 'e';
	color[2] = 'd';
	color[3] = '\0';	

	printf("Our first string is %s\n",color);

	//color_ptr[2] = '\0';
	printf("Our pointer to first string is %s\n",color_ptr);

	// Compiler just gives you a pointer and points it at the literal
	char * day = "Monday";

	// Cannot modify it done this way
	//day[0] = 'T';
	printf("Our string is %s\n",day);

	printf("Let us copy strings!\n");
	strncpy(color,day,4);

	printf("Color after our copy is:\n");
	printf("Color: %s\n",color);

	printf("Let us try that again!\n");

	strncpy(color,day,7);
	printf("Color after our copy is:\n");
	printf("Color: %s\n",color);
}
