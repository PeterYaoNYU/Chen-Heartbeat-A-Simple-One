#include "server.h"

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
    // this value will be used in the recvfrom function
    clnt_adr_sz = sizeof(clnt_adr);

    for (i = 0; i < NUM_PKT; i++){
        recvfrom(serv_sock, (char*)recv_pkt, sizeof(pkt), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        // pkt received, timestamp it!
        gettimeofday(&pkt_arrive_date, NULL);
        latency[ntohl(recv_pkt->id)] = 1000000 * (pkt_arrive_date.tv_sec- ntohl(recv_pkt->send_time.tv_sec)) + pkt_arrive_date.tv_usec - ntohl(recv_pkt->send_time.tv_usec);
        // debug purpose
        printf("pkt id: %d, latency: %ld\n", ntohl(recv_pkt->id), 1000000 * (pkt_arrive_date.tv_sec- ntohl(recv_pkt->send_time.tv_sec)) + pkt_arrive_date.tv_usec - ntohl(recv_pkt->send_time.tv_usec));
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

    // enter the heartbeat state
    // init the need memory and value
    long count = 0;
    struct timeval ea;
    ea.tv_sec = 0;
    ea.tv_usec = 0;
    struct timeval * arrival_times = (struct timeval*)malloc(N*sizeof(struct timeval));
    memset(arrival_times, 0, N * sizeof(time_t));
    // int start = 0;
    int end = 0;
    struct timeval heartbeat_period;
    heartbeat_period.tv_sec = 1;
    heartbeat_period.tv_usec = 10000;
    long leftover = 0;
    // safety margin is in terms of microsecond, init to the mean_latency
    // safety margin is treated as a constant in Chen's Algorithm
    long safety_margin = mean_latency;
    clnt_state = ALIVE;
    while (1){
        // reveive the heatbeat packet
        str_len = recvfrom(serv_sock, (void*)recv_pkt, sizeof(pkt), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if (str_len <= 0){
            continue;
        }
        gettimeofday(&pkt_arrive_date, NULL);
        if (count>N){
            // got to update the estimated arrival time, calculating on the fly
            if (clnt_state==DOWN){
                clnt_state = ALIVE;
                printf("CLIENT BACK ONLINE\n");
            }
            struct timeval temp;
            temp.tv_sec = pkt_arrive_date.tv_sec - arrival_times[end].tv_sec;
            temp.tv_usec = pkt_arrive_date.tv_usec - arrival_times[end].tv_usec;
            temp.tv_usec += MILLION * (temp.tv_sec % N);
            temp.tv_sec /= N;
            temp.tv_usec /= N;
            ea.tv_sec+=temp.tv_sec;
            ea.tv_usec +=temp.tv_usec;
            ea.tv_sec+= (N-1) * heartbeat_period.tv_sec / N;
            ea.tv_usec += (N-1) * heartbeat_period.tv_usec / N;
            set_up_itimer(ea.tv_sec, ea.tv_usec, safety_margin);
            printf("pkt %d received, next arrival estimate: sec %ld, usec %ld\n", ntohl(recv_pkt->id), ea.tv_sec, ea.tv_usec);

        } else if (count == N){
            // enough packets gathered, making first estimate and initialize the signal handler 
            count++;         
            arrival_times[end] = pkt_arrive_date;
            end = (end+1) % N;
            // sum the arrival times
            for (i = 0; i < N; i++){
                ea.tv_sec+=arrival_times[i].tv_sec;
                ea.tv_usec += arrival_times[i].tv_usec;
            } 
            // init the estimated arrival time
            leftover = ea.tv_sec % N;
            ea.tv_sec = ea.tv_sec / N;
            ea.tv_usec = MILLION * leftover + ea.tv_usec / N;
            ea.tv_sec += count * heartbeat_period.tv_sec;
            ea.tv_usec += count * heartbeat_period.tv_usec;
            set_up_interrupt();
            set_up_itimer(ea.tv_sec, ea.tv_usec, safety_margin);
        } else if (count < N){
            // have not received enough packets to put an estimate
            count++;
            arrival_times[end] = pkt_arrive_date;
            end = (end+1) % N;
            printf("received, but not enough packets to estimate\n");
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
    printf("the time now is %ld, %ld\n", now.tv_sec, now.tv_usec);
    struct itimerval value;
    value.it_interval.tv_sec = sec-now.tv_sec;
    value.it_interval.tv_usec = usec - now.tv_usec + safety_margin;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_REAL, &value, NULL));
}

