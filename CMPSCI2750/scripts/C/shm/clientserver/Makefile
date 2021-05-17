CC		= gcc
CFLAGS		= -g -Wall
SERVER		= server
CLIENT		= client
MASTER		= master
SERVER_OBJ	= server.o
CLIENT_OBJ	= client.o
MASTER_OBJ	= master.o
OBJ		= util.o
TARGETS		= $(SERVER) $(CLIENT) $(MASTER)

.SUFFIXES: .c .o

all: $(TARGETS)

$(SERVER): $(SERVER_OBJ) $(OBJ)
	$(CC) -o $@ $(SERVER_OBJ) $(OBJ)

$(CLIENT): $(CLIENT_OBJ) $(OBJ)
	$(CC) -o $@ $(CLIENT_OBJ) $(OBJ)

$(MASTER): $(MASTER_OBJ) $(OBJ)
	$(CC) -o $@ $(MASTER_OBJ) $(OBJ)

.c.o:
	$(CC) -c $(CFLAGS) $<

$(SERVER_OBJ): defs.h

.PHONY: clean
clean:
	rm -rf $(TARGETS) *.o *~
