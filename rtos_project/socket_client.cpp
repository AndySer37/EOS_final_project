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

Role *role;

void *recv_handler(void *arg){
    char buf_rcv[300]= {};
    while(1){
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
            printf("%s\n", buf_rcv);
            close(sockfd);
            flag_shutdown = true;
            pthread_exit(0);
        }else if(strstr(buf_snd, "--")){            // if receive '--' command
            int r, p, amo;
            sscanf(buf_rcv, "--role %d --p %d --amo %d", &r, &p, &amo);
            role = new Role(sockfd, r, p, amo);
            pthread_exit(0);
        }else{
            printf("%c[2K\r", 27);
            printf("%s\r\n%s", buf_rcv, buf_snd); // Bug: cannot record previous words after receving msg from server 
        }
    }
}

int getch(void){
    int ch;
    struct termios oldt;
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt); /*store old settings */
    newt = oldt; /* copy old settings to new settings */
    newt.c_lflag &= ~(ICANON | ECHO); /* make one change to old settings in new settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); /*apply the new settings immediatly */
    ch = getchar(); /* standard getchar call */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /*reapply the old settings */
    return ch; /*return received char */
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
    char username[30] = {};
    printf("Username: ");
    sprintf(username, "%c%c", rand()%20+'A', rand()%20+'A');
    // scanf("%s", username);
    ret = send(sockfd, username, strlen(username), 0);
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
    while(!flag_shutdown){
        char x,y,z;
        x = getch();

        if (x == 27){
            y = getch();
            z = getch();
            if (y == 91){
                switch (z){
                    case 65:
                        printf("%c[2K\r", 27);
                        printf("up");
                        sprintf(buf_snd, "up");
                        break;
                    case 66:
                        printf("%c[2K\r", 27);
                        printf("down");
                        sprintf(buf_snd, "down");
                        break;
                    // case 67:
                    //     printf("%c[2K\r", 27);
                    //     printf("right\r");
                    //     break;
                    // case 68:
                    //     printf("%c[2K\r", 27);
                    //     printf("left\r");
                    //     break;
                }
            }
        }
        else if(x == 8 || x == 127){
            printf("\b \b");
            fflush(stdout);
            sprintf(buf_snd, "%s\b \b", buf_snd);
        }
        else if(x == 10){
            printf("%c", x);
            // printf("%s\n", buf_snd);
            int ret = send(sockfd, buf_snd, strlen(buf_snd), 0);
            if(0 > ret){
                perror("send");
                return -1;
            }
            memset(buf_snd, 0, sizeof(buf_snd));
        }
        else if(x != -1){
            printf("%c", x);
            sprintf(buf_snd, "%s%c", buf_snd, x);
        }
    }
    return 0;
}