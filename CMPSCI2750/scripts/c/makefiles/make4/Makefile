# Makefile with shared library creation

CC	= gcc
CFLAGS	= -g
TARGET	= math
OBJS	= main.o
LIBOBJS = square.o cube.o foo.o
LIB	= mymath.so

ALL:	$(LIB) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIB)

$(LIB): $(LIBOBJS)
	$(CC) -shared -Wl,-soname,$@ -o $@ $(LIBOBJS)

square.o: square.c
	$(CC) -fpic -c square.c

cube.o: cube.c
	$(CC) -fpic -c cube.c

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	/bin/rm -f *.o *~ $(LIB) $(TARGET)
