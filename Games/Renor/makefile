CC	= gcc
CFLAGS	= -g
TARGET	= renor
OBJS	= renor.o

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

renor.o: renor.c
	$(CC) $(CFLAGS) -c renor.c

.PHONY: clean
clean:
	/bin/rm -f *.o $(TARGET)
