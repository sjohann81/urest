CC      = gcc
CFLAGS  = -O2 -Wall

all:
	$(CC) $(CFLAGS) urest.c base32.c server.c -o server
	$(CC) $(CFLAGS) urest.c base32.c client.c -o client
	
clean:
	-rm -f *.o *~ server client
