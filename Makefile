CFLAGS = -Wall -Wextra -Werror -g -lpthread -lrt

all: dine

dine: dine.o
	gcc $(CFLAGS) -o dine dine.o

dine.o: dine.c dine.h
	gcc $(CFLAGS) -c dine.c -o dine.o

clean:
	rm -f *.o *.txt dine