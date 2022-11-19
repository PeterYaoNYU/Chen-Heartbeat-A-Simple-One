all: runserver

server: heartbeat_udp_server.c server.h
	gcc -o server -g -Wall heartbeat_udp_server.c -lm

client: client_udp.c
	gcc -o client -g -Wall client_udp.c 

runserver: server client
	./server 8002

clean:
	rm -f ./client, ./server