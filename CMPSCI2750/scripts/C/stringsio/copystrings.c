#include <stdio.h>

void copy_string ( char * color_ptr, char * const color) {
	char * in = color;
	char * out = color_ptr;

	while ( *out++ = *in++);
}


int main() {

	// Compiler allocates the space and copies it over
	char color[] = "blue";
	char * color_ptr = color;

	printf("Our first string is %s\n",color);
	printf("Our pointer to first string is %s\n",color_ptr);

	// can modify color
	color[0] = 'r';
	color[1] = 'e';
	color[2] = 'a';
	color[3] = 'd';	
	color[4] = '\0';	

	printf("Our first string is %s\n",color);
	printf("Our pointer to first string is %s\n",color_ptr);

	// Compiler just gives you a pointer and points it at the literal
	char * day = "Monday";

	// Cannot modify it done this way
	//	day[0] = 'T';
	printf("Our string is %s\n",day);

	printf("Let us copy strings!\n");
	copy_string(color,day);

	printf("Color after our copy is:\n");
	printf("Color: %s\n",color);

}
