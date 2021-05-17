# Makefile with suffix rules

CC	= gcc
CFLAGS	= -g
TARGET	= math
OBJS	= main.o
LIBOBJS = square.o cube.o
LIBS	= -lmymath
MYLIBS	= libmymath.a
LIBPATH	= .
.SUFFIXES: .c .o

$(TARGET): $(OBJS) $(MYLIBS)
	$(CC) -o $@ -L${LIBPATH} $(OBJS) $(LIBS)

$(MYLIBS): $(LIBOBJS)
	ar -rs $@ $(LIBOBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	/bin/rm -f *.o *~ $(TARGET) $(MYLIBS)
