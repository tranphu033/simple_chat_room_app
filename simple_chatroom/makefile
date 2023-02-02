CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm 

all: client server

client: client-socket.o
	${CC} client-socket.o -o client -pthread

client-socket.o: client-socket.c
	${CC} ${CFLAGS} client-socket.c

server: server-socket.o
	${CC} server-socket.o -o server -pthread

server-socket.o: server-socket.c
	${CC} ${CFLAGS} server-socket.c

clean:
	rm -f *.o *~
