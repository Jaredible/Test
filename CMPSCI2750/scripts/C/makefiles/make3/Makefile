# Makefile with suffix rules

CC	= gcc
CFLAGS	= -g
TARGET	= math
OBJS	= main.o square.o cube.o quad.o
.SUFFIXES: .c .o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

$.o:	%.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	/bin/rm -f *.o $(TARGET)
