#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 1024
void error_handling(char * msg);

int main(int argc, char * argv[])
{
    int serv_sock;
    char message[BUFSIZE];
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

    while(1){
        clnt_adr_sz = sizeof(clnt_adr);
        recvfrom(serv_sock, &message, BUFSIZE, 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        printf("%s\n", message);
    }
    close(serv_sock);
    return 0;
}

void error_handling(char * message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}