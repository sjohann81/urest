CC = gcc
CFLAGS = -Wall -O2
AR = ar
ARFLAGS = rcs liburest.a

all: client server

client: client.o lib_urest
	$(CC) $(CFLAGS) -o client client.o -L. -lurest

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

server: server.o lib_urest
	$(CC) $(CFLAGS) -o server server.o -L. -lurest

server.o: client.c
	$(CC) $(CFLAGS) -c server.c
	
	
lib_urest: base32.o urest.o
	$(AR) $(ARFLAGS) base32.o urest.o

urest.o: urest.c
	$(CC) $(CFLAGS) -c urest.c

base32.o: base32.c
	$(CC) $(CFLAGS) -c base32.c
	
clean:
	-rm -f *.o *.a *~ server client
