#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>
// is clock drift a problem here?
// consider the n most recent heartbeat

#define N 5
#define BUFSIZE 1024
#define NUM_PKT 100
#define MILLION 1000000L

enum states{ALIVE, DOWN};
enum states clnt_state;


typedef struct test_network_packet{
    int id;
    struct timeval send_time;
} pkt;

void error_handling(char * msg);

void my_handler(int s);

int set_up_interrupt();

int set_up_itimer(long sec, long usec, long safety_margin);

