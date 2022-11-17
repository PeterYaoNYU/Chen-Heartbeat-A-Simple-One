#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BUFSIZE 1024
// void error_handling(char * message);

typedef struct test_network_packet{
    int id;
    struct timeval send_time;
} pkt;

int main(int argc, char * argv[]){
    // char message[BUFSIZE];
    int sock;
    // socklen_t adr_sz;
    int i, num_to_send;
    pkt * test = (pkt*)malloc(sizeof(pkt));

    char * server_ip = "43.142.238.147";

    // struct sockaddr_in serv_adr, from_adr;
    struct sockaddr_in serv_adr;
    if (argc!=2){
        printf("usage: %s <port_number>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(server_ip);
    serv_adr.sin_port = htons(atoi(argv[1]));

    for (i = 0; i < 1000; i++){
        num_to_send = htonl(i);
        test->id = num_to_send;
        // is clock drift a problem here?
        // udp packet loss?
        gettimeofday(&(test->send_time), NULL);
        test->send_time.tv_sec = htonl(test->send_time.tv_sec);
        test->send_time.tv_usec = htonl(test->send_time.tv_usec); 
        sendto(sock, (char*)test, sizeof(pkt), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
        // printf("the number sent: %d\n", ntohl(num_to_send));
        printf("the msg sent: %d\n", ntohl(test->id));
        printf("the time sent: %d\n", ntohl(test->send_time.tv_usec));
    }
    printf("sock closing\n");
    close(sock);
    return 0;
}