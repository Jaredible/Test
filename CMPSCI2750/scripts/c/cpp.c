// #include <stdio.h>

#define LEN 100

typedef enum
{
    clubs,
    diamonds,
    hearts,
    spades
} suit_t;

typedef struct
{
    unsigned int	number	: 4;
    suit_t		suit	: 2;
} card_t;

/* Program to illustrate preprocessor */

int main()
{
    printf ( "Card type size is %d\n", sizeof ( card_t ) );
    printf ( "%d\n", LEN * LEN );
}
