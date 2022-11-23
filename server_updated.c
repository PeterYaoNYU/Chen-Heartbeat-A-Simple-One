#include "server_updated.h"

int main(int argc, char * argv[])
{
    // set up the UPD socket
    int serv_sock;
    int str_len;
    // to give an estimate of the packet loss situation
    int lost_packet_count = 0;
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
    int i;
    // for (i = 0; i < NUM_PKT; i++){
    //     recvfrom(serv_sock, (char*)recv_pkt, sizeof(pkt), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
    //     // pkt received
    //     printf("pkt id: %d, the time when sent: %d, %d\n", ntohl(recv_pkt->id), ntohl(recv_pkt->send_time.tv_sec), ntohl(recv_pkt->send_time.tv_usec));
    // }




    // enter the heartbeat state
    // initialization
    int k=-1; //the largest sequence number in all the messages host receives so far 
    struct timeval ea; //estimated arrival time
    ea.tv_sec = 0;
    ea.tv_usec = 0;
    long alpha = 0;
    clnt_state = DOWN; // initially, all client processes will be suspected
    int j = 0; // to keep track of the current sequence number 

    // allocate an array to hold the n most recent packets arrival dates
    struct timeval * arrival_times = (struct timeval*)malloc(N*sizeof(struct timeval));
    // a pointer to the topmost positions of the array
    int end = 0;
    // set up the interrupt handler for alarms 
    set_up_interrupt();
    // start infinite looping of the server 
    while (1){
        str_len = recvfrom(serv_sock, (void*)recv_pkt, sizeof(pkt), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if (str_len <= 0){
            continue;
        }
        // timestamp the receive date 
        gettimeofday(&pkt_arrive_date, NULL);
        j = ntohl(recv_pkt->id);
        arrival_times[end] = pkt_arrive_date;
        end = (end+1) % N;
        if (j>k){
            k = j;
            if (j < N){
                printf("not enough packets received to make an estimate\n");
            } else {
                // i have no idea how you would handle the packet loss
                // assume that the network is stable
                struct timeval temp;
                temp.tv_sec = 0;
                temp.tv_usec = 0;
                for (i = 0; i < N; i++){
                    temp.tv_sec += arrival_times[i].tv_sec;
                    temp.tv_usec += arrival_times[i].tv_usec;
                }
                temp.tv_sec = temp.tv_sec-DELTAI_TVSEC * (j-N);
                temp.tv_usec = temp.tv_usec -DELTAI_TVUSEC * (j-N);
                temp.tv_usec += MILLION * (temp.tv_sec % N);
                temp.tv_sec /= N;
                temp.tv_usec /= N;
                temp.tv_sec += (j+1)* DELTAI_TVSEC;
                temp.tv_usec += (j+1) * DELTAI_TVUSEC;
                ea = temp;
                set_up_itimer(ea.tv_sec, ea.tv_usec, alpha);
                }
            if (clnt_state==DOWN){
                clnt_state = ALIVE;
                printf("CLIENT BACK ONLINE\n");
            }
        }
    }

    close(serv_sock);
    return 0;
}

void error_handling(char * message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void my_handler(int s){
    clnt_state = DOWN;
    printf("doubt that the client is down\n");
    return;
}

int set_up_interrupt(){
    struct sigaction act;
    act.sa_handler = my_handler;
    act.sa_flags =0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGALRM, &act, NULL));
}

int set_up_itimer(long sec, long usec, long safety_margin){
    struct timeval now;
    gettimeofday(&now, NULL);
    printf("the time now is %ld, %ld; expected arrival of the next packet: %ld, %ld\n", now.tv_sec, now.tv_usec, sec, usec);
    struct itimerval value;
    value.it_interval.tv_sec = sec-now.tv_sec;
    value.it_interval.tv_usec = usec - now.tv_usec + safety_margin;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_REAL, &value, NULL));
}