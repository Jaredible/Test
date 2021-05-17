CC	= gcc -pg
CFLAGS	= -g -Wall
PROG	= mychmod
OBJS	= mychmod.o

.SUFFIXES: .c .o

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS)

.c.o:
	$(CC) -c $(CFLAGS) $<

.PHONY: clean
clean:
	/usr/bin/rm -f *.o $(PROG) core
