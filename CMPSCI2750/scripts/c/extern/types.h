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

int square ( int );

