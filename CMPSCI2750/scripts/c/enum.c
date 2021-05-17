/******************************************************************************/
/* Using an enumeration type                                                  */

#include <stdio.h>

// Declare a new enumerated type month_t
// with all the months in the year

typedef enum
{
    JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC
} month_t;


int main()
{
    month_t month;
    char * month_name[] = { "", "January", "February", "March", "April", \
                            "May", "June", "July", "August", "September", \
                            "October", "November", "December" };

    for ( month = JAN; month <= DEC; month++ )
        printf ( "%02d%11s\n", month, month_name[month] );

    return ( 0 );
}
/******************************************************************************/
