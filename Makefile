CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -g
LDFLAGS = -pthread

all: servidor cliente

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c $(LDFLAGS)

cliente: cliente.c
	$(CC) $(CFLAGS) -o cliente cliente.c $(LDFLAGS)

clean:
	rm -f servidor cliente *.o

run-servidor: servidor
	./servidor

run-cliente: cliente
	./cliente 127.0.0.1

.PHONY: all clean run-servidor run-cliente
