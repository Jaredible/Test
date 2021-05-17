#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"

char time_in_sec[10];

void die ( char * prog, char * fn )
{
    char line[80];
    sprintf ( line, "Error: %s: %s", prog, fn );
    perror ( line );
    exit ( EXIT_FAILURE );
}

char *  print_time()
{
    struct tm * cur_time;
    time_t t = time(NULL);
    cur_time = localtime (&t);
    sprintf ( time_in_sec, "%02d:%02d:%02d", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec );

    return ( time_in_sec );
}
