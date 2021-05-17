CC	= gcc
CFLAGS	= -g
PROG	= cpp
OBJS	= cpp.o square.o

.SUFFIXES: .c .o

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS)

.c.o:
	$(CC) -c $(CFLAGS) $<

.PHONY: clean

clean:
	/bin/rm -f $(PROG) *.o core
