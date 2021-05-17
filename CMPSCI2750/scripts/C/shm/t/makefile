
all:	shm_write shm_read

shm_write:	shm_write.o
	gcc -o shm_write shm_write.o

shm_read:	shm_read.o
	gcc -o shm_read shm_read.o

shm_read.o:	shm_read.c
	gcc -c shm_read.c

shm_write.o:	shm_write.c
	gcc -c shm_write.c

clean:
	rm -f *.o
