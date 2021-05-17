CC	= gcc
CFLAGS	= -g
INCLUDEDIR = .
C_OBJS	= create.o
R_OBJS	= read.o
U_OBJS	= update.o
PROGS	= create read update
HEADERS	= types.h

all:	$(PROGS)

create:	${C_OBJS}
	${CC} ${CFLAGS} -o $@ ${C_OBJS}

read:	${R_OBJS}
	${CC} ${CFLAGS} -o $@ ${R_OBJS}

update:	${U_OBJS}
	${CC} ${CFLAGS} -o $@ ${U_OBJS}

.SUFFIXES: .c .o

.c.o:
	${CC} ${CFLAGS} -c -I$(INCLUDEDIR) $<

.PHONY: clean

clean:
	/bin/rm -f *.o $(PROGS) core

$(C_OBJS): $(HEADERS)
$(R_OBJS): $(HEADERS)
$(U_OBJS): $(HEADERS)
