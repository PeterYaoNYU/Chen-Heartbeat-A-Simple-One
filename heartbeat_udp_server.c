#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define BUFSIZE 1024
#define NUM_PKT 100
#define MILLION 1000000L
void error_handling(char * msg);

typedef struct test_network_packet{
    int id;
    struct timeval send_time;
} pkt;

int main(int argc, char * argv[])
{
    int serv_sock;
    // char message[BUFSIZE];
    // int message;
    int str_len;
    socklen_t clnt_adr_sz;
    struct sockaddr_in serv_adr, clnt_adr;
    if (argc!=2){
        printf("include port number, exiting\n");
        exit(1);
    }
    serv_sock=socket(PF_INET, SOCK_DGRAM, 0);
    if(serv_sock == -1){
        error_handling("UDP socket creating error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    pkt * recv_pkt = (pkt*)malloc(sizeof(pkt));
    struct timeval pkt_arrive_date;
    long * latency = (long *)malloc(100 * sizeof(long));

    while(1){
        clnt_adr_sz = sizeof(clnt_adr);
        recvfrom(serv_sock, (char*)recv_pkt, sizeof(pkt), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        // pkt received, timestamp it!
        gettimeofday(&pkt_arrive_date, NULL);
        latency[ntohl(recv_pkt->id)] = 1000000 * (pkt_arrive_date.tv_sec- ntohl(recv_pkt->send_time.tv_sec)) + pkt_arrive_date.tv_usec - ntohl(recv_pkt->send_time.tv_usec);
        // debug purpose
        printf("pkt id: %d, latency: %ld\n", ntohl(recv_pkt->id), 1000000 * (pkt_arrive_date.tv_sec- ntohl(recv_pkt->send_time.tv_sec)) + pkt_arrive_date.tv_usec - ntohl(recv_pkt->send_time.tv_usec));
        // printf("%d\n", ntohl(recv_pkt->id));
        // printf("%d\n", ntohl(recv_pkt->send_time.tv_usec));
    }
    close(serv_sock);
    return 0;
}

void error_handling(char * message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}