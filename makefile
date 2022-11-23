all: runserver

server: server_updated.c server_updated.h
	gcc -o server -g -Wall server_updated.c -lm

client: client_udp.c
	gcc -o client -g -Wall client_udp.c 

runserver: server client
	./server 8002

clean:
	rm -f ./client, ./server