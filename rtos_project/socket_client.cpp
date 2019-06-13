#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <string.h>
#include <termios.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sstream>
#include <fstream> 
#include <vector>
#include <iomanip>
#include "sockop.c"
#include <syslog.h>
#include <cstdio>
#include <ctime>
#include "role.h"

using namespace std;

typedef struct sockaddr *sockaddrp;
int sockfd;
char buf_snd[255] = {0};                // temporary key in buffer
volatile bool flag_shutdown = false;    
string game_info = "";
Role *role;
bool game_start = false;

void *recv_handler(void *arg){
    char buf_rcv[255]= {};
    while(1){
        memset(buf_rcv, 0, sizeof(buf_rcv));
        int ret = recv(sockfd, buf_rcv, sizeof(buf_rcv), 0);
        if(ret < 0){
            perror("recv");
            flag_shutdown = true;
            return 0;
        }
        if(ret == 0){
            close(sockfd);
            flag_shutdown = true;
            pthread_exit(0);
        }else if(strstr(buf_rcv, "SHUTDOWN")){      // if shutdown
            // printf("%s\n", buf_rcv);
            ss_win<<buf_rcv<<endl;
            w.recv_msg(1, ss_win);
            close(sockfd);
            flag_shutdown = true;
            pthread_exit(0);
        }else if(strstr(buf_rcv, "Players infomation")){      // if shutdown
            // printf("%s\n", buf_rcv);
            // ss_win<<buf_rcv<<endl;
            // w.recv_msg(1, ss_win);
            game_info = string(buf_rcv);
            flag_shutdown = true;
        }else if(strstr(buf_rcv, "--")){            // if receive '--' command
            int r, p, amo;
            sscanf(buf_rcv, "--role %d --p %d --amo %d", &r, &p, &amo);
            role = new Role(sockfd, r, p, amo);
            role->game_start_update(game_info);
            game_start = true;
            pthread_exit(0);
        }else{
            // printf("%s\r\n%s", buf_rcv, buf_snd); // Bug: cannot record previous words after receving msg from server 
            ss_win<<buf_rcv<<endl;
            w.recv_msg(1, ss_win);
            // ss_win<<buf_snd<<endl;
            // w.recv_msg(1, ss_win);
        }
    }
}

int main(int argc,char **argv){
    srand(time(NULL));
    if(argc != 3){
        perror("./socket_client [PORT] [SERVER_IP]");
        return -1;
    }

    // Allocate socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
        perror("socket");
        return -1;
    }

    // Input port and ip from args
    struct sockaddr_in addr = {AF_INET};
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = inet_addr(argv[2]);
    socklen_t addr_len = sizeof(addr);

    // Connect 
    int ret = connect(sockfd,(sockaddrp)&addr,addr_len);
    if(0 > ret){
        perror("connect");
        return -1;
    }

    // Send username once connecting server successfully  
    string username;
    // printf("Username: ");
    // ss_win<<"Username: "<<endl;
    // w.recv_msg(1, ss_win);
    // sprintf(username, "%c%c", rand()%20+'A', rand()%20+'A');
    // scanf("%s", username);
    username = w.input();
    ret = send(sockfd, username.c_str(), username.size(), 0);
    if(0 > ret){
        perror("connect");
        return -1;
    }

    // Socket receiver thread
    pthread_t tid;
    ret = pthread_create(&tid, NULL, recv_handler, NULL);
    if(0 > ret){
        perror("pthread_create");
        return -1;
    }

    // Continue typing
    while(!flag_shutdown && !game_start){
        usleep(50000);
    }
    role->save_ptr(role);
    return 0;
}