CC         = gcc
CFLAGS     = -Wall -g

STANDARD   = shared.h
SRC        = queue.h linklist.h
MASTER_SRC = oss.c
USER_SRC   = user.c

OBJ        = queue.o linklist.o
MASTER_OBJ = $(MASTER_SRC:.c=.o)
USER_OBJ   = $(USER_SRC:.c=.o)

MASTER     = oss
USER       = user

OUTPUT    = $(MASTER) $(USER)
all: $(OUTPUT)


%.o: %.c $(STANDARD) $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(MASTER): $(MASTER_OBJ) $(OBJ)
	$(CC) $(CFLAGS) $(MASTER_OBJ) $(OBJ) -o $(MASTER)

$(USER): $(USER_OBJ)
	$(CC) $(CFLAGS) $(USER_OBJ) -o $(USER)


.PHONY: clean
clean:
	/bin/rm -f $(OUTPUT) *.o *.dat

