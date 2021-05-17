#include <iostream>

// $Author: bhatias $
// $Date: 2012/02/23 23:49:33 $
// $Log: hello.c,v $
// Revision 2.1  2012/02/23 23:49:33  bhatias
// Changed it into a C++ program.
//
// Revision 1.5  2012/02/23 23:40:33  bhatias
// Added RCS ID for the executable.
//
// Revision 1.4  2012/02/22 00:43:08  bhatias
// Added RCS keywords.
//

// My second program to say hello to the world.

char RCSID[] = "$Author: bhatias $";

int main ( int argc, char ** argv )
{
    if ( argc < 2 )
	std::cout << "Hello world" << std::endl;
    else
	if ( *argv[1] == 'r' )
	    std::cout << "RCS ID: " << RCSID << std::endl;
	else
	    std::cout << "Hello " << argv[1] << std::endl;

    return ( 0 );
}
