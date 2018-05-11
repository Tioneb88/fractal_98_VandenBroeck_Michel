CC=gcc -Wall -g

all:	main
	make clean

main:	main.o
	cd libfractal && make
	$(CC)	-o	main	main.o	libfractal/libfractal.a	-lSDL -lpthread	

main.o:	main.c
	$(CC)	-c	main.c	-lpthread

lib:
	cd libfractal && make
	make clean

clean:
	rm	-rf	.o	libfractal/.o	libfractal/libfractal.a test

tests: test.o
	$(CC) -o test test.o -lcunit
	./test
test.o: tests/test.c
	$(CC) -o test.o -c tests/test.c

.SILENT:
