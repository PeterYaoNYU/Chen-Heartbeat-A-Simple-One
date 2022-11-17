        // is clock drift a problem here?
        // udp packet loss?
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <math.h>

#define BUFSIZE 1024
#define NUM_PKT 1000
#define MILLION 1000000L
void error_handling(char * msg);

typedef struct test_network_packet{
    int id;
    struct timeval send_time;
} pkt;

int main(int argc, char * argv[])
{
    int serv_sock;
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
    long * latency = (long *)malloc(NUM_PKT * sizeof(long));
    // clear the memory to 0 to see if there is a packet missing
    memset(latency, 0, sizeof(long)*NUM_PKT);
    long mean_latency = 0;
    long std_dev_latency = 0;
    int i;

    for (i = 0; i < NUM_PKT; i++){
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
    // calculate the mean latency to determine the safety margin
    for (i = 0; i < NUM_PKT; i++){
        mean_latency += latency[i]/NUM_PKT;
    }
    // calculate the std deviation to determine the dafety margin
    for (i = 0; i < NUM_PKT; i++){
        std_dev_latency += pow(latency[i] - mean_latency, 2);
    }
    std_dev_latency = sqrt(std_dev_latency/NUM_PKT);
    printf("the stddev is %ld, and the mean is %ld\n", std_dev_latency, mean_latency);
    close(serv_sock);
    return 0;
}

void error_handling(char * message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}