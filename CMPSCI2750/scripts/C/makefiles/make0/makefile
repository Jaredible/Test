
checkeven:	main.o functions.o
	gcc -o checkeven main.o functions.o

main.o:	main.c functions.h
	gcc -g -c main.c

functions.o:	functions.c functions.h
	gcc -g -c functions.c
