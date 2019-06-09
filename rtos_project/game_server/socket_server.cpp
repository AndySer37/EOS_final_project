#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "game_server.h"

using namespace std;
#define MAX_SOCKET_CONNECTION 50
#define SOCKET_PORT "8787"
#define SOCKET_IP "127.0.0.1"

typedef struct sockaddr *sockaddrp;
typedef enum{
    IDLE_STATE,
    LOADING_STATE,
    LOGIN_STATE,
    AWARD_STATE,

    MORNING_CHAT,
    MORNING_VOTE,
    MORNING_EFFECT,

    EVENING_CHAT,
    EVENING_VOTE,
    EVENING_EFFECT

}GameState;
volatile GameState game_state;

struct sockaddr_in src_addr[MAX_SOCKET_CONNECTION];    //to record client ip
socklen_t src_len = sizeof(src_addr[0]);
int confd[MAX_SOCKET_CONNECTION] = {0};    // to record file descriptors of socket
int num_conn = 0;                          // to record how many people are joining in game room
volatile unsigned cnt_systick = 0;                    // to count system tick timer
clock_t t_gamestart, t_stamp;



int socket_init(void){
    // Allocate socket
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
        perror("socket");
        return -1;
    }

    // Input port and ip from args 
    struct sockaddr_in addr = {AF_INET};
    addr.sin_port = htons(atoi(SOCKET_PORT));    // Convert host btye order to network byte order
    addr.sin_addr.s_addr = inet_addr(SOCKET_IP);
    socklen_t addr_len = sizeof(addr);

    //socket bind
    int ret = bind(sockfd, (sockaddrp)&addr, addr_len);
    if(ret < 0) {
        perror("bind");
        return -1;
    }

    // Set Max number of socket connection
    listen(sockfd, MAX_SOCKET_CONNECTION);
    return sockfd;
}

void *socket_rcv_handler(void *indexp){
    int userid = *(int *)indexp;
    char buf_rcv[255] = {};
    char buf_snd[255] = {};
    char username[20] = {};

    // Once user login to the game, get the username from clients
    int ret = recv(confd[userid], username, sizeof(username), 0);
    if(ret < 0){
        perror("recv");
        close(confd[userid]);
        return 0;
    }

    // GameState check: login
    if(game_state != LOGIN_STATE){
        time(&t_stamp);
        sprintf(buf_snd, "Sorry, the game has been started for %.0lf %s(s). Please wait for next round\n", 
            (difftime(t_stamp, t_gamestart)<60)? difftime(t_stamp, t_gamestart):difftime(t_stamp, t_gamestart)/60,
            (difftime(t_stamp, t_gamestart)<60)? "sec": "min");
        send(confd[userid], buf_snd, strlen(buf_snd), 0);
        confd[userid] = -1;
        pthread_exit(0);
    }else cout << "Welcome [" << username << "] to the game room." << endl;
    

    // A loop to deal with the messages from clients
    while(1){
        bzero(buf_rcv, sizeof(buf_rcv));    // clear rcv buffer
        recv(confd[userid], buf_rcv, sizeof(buf_rcv), 0);
        
        if(game_state == MORNING_CHAT || game_state == EVENING_CHAT || game_state == LOGIN_STATE){
            // User quit
            if(strlen(buf_rcv) == 0/* or strcmp("quit",buf_rcv) == 0*/){
                sprintf(buf_snd, "[%s] quit the game", username);
                cout << buf_snd << endl;
                for(int i = 0;i <= num_conn; i++){
                    if(userid == i || confd[i] == 0) continue;  // skip the user who quit the game
                    send(confd[i], buf_snd, strlen(buf_snd), 0);
                }
                confd[userid] = -1;
                pthread_exit(0);
            }

            // broadcast to each other
            sprintf(buf_snd, "[%s]: %s", username, buf_rcv);
            cout << buf_snd << endl;
            for(int i = 0; i <= num_conn; i++){
                if(i == userid || 0 == confd[i]) continue;    // skip the user who send the msg
                send(confd[i], buf_snd, sizeof(buf_snd), 0);
            }
        }
        else if(game_state == MORNING_EFFECT || game_state == EVENING_EFFECT){

        }
    }
}

void systick_handler(int iSignNo){
    cnt_systick++;
}

int systick_init(void){
    struct sigevent evp;  
    struct itimerspec ts;  
    timer_t timer;  
    int ret;
    evp.sigev_value.sival_ptr = &timer;  
    evp.sigev_notify = SIGEV_SIGNAL;  
    evp.sigev_signo = SIGUSR1;  
    signal(evp.sigev_signo, systick_handler); 
    ret = timer_create(CLOCK_REALTIME, &evp, &timer);  
    if(ret) {
        perror("timer_create");
        return -1;
    } 
    ts.it_interval.tv_sec = 1; // the spacing time  
    ts.it_interval.tv_nsec = 0;  
    ts.it_value.tv_sec = 0;  // the delay time start
    ts.it_value.tv_nsec = 0;
    cout << "SysTick start..." << endl;
    ret = timer_settime(timer, 0, &ts, NULL);  
    if(ret) {
        perror("timer_settime"); 
    } 
}

void *users_login_handler(void *socketfd){
    int userid = 0;
    int sockfd = *(int *)socketfd;
    while(num_conn <= MAX_SOCKET_CONNECTION){
        confd[num_conn] = accept(sockfd, (sockaddrp)&src_addr[num_conn], &src_len);
        num_conn++;
        userid = num_conn-1; // set userid in order of login

        pthread_t tid;
        int ret = pthread_create(&tid, NULL, socket_rcv_handler, &userid);
        if(ret < 0){
            perror("pthread_create");
            return 0;
        }
        usleep(500000);
    }
}

void wait_timer_countdown(unsigned int sec){
    cnt_systick = 0;

    char buf_snd[100] = {};
    while(cnt_systick < sec){
        if(sec - cnt_systick <= 5){
            sprintf(buf_snd, "[SYSTEM] count down %d sec(s)", sec - cnt_systick);
            cout << buf_snd << endl;
            for(int i = 0; i <= num_conn; i++)
                send(confd[i], buf_snd, sizeof(buf_snd), 0);
        }
        cnt_systick++;
        usleep(900000);
    }
}

int main(int argc, char *argv[]){
    // socket init
    int sockfd = socket_init();
    // systick init
    int ret = systick_init();

    game_state = LOADING_STATE;
    cout << "Game Loading......";
    // create a thread as user login handler
    pthread_t tid;                  
    ret = pthread_create(&tid, NULL, users_login_handler, (void*) &sockfd);
    if(ret < 0){
        perror("pthread_create");
        return -1;
    }
    // create game server object
    int** tb = new int*[ROLE_AMO];
    for (int i = 0;i < ROLE_AMO; i++){
        tb[i] = new int [3];
        tb[i][0] = i;
        tb[i][1] = DEFAULT;
        tb[i][2] = 1;
    }
    game_server game_server(11, tb);
    

    game_state = LOGIN_STATE;
    cout << "Done" << endl;
    cout << "User Login state......" << endl;
    wait_timer_countdown(20);


    game_state = MORNING_CHAT;
    cout << "User CHAT state......" << endl;
    time(&t_gamestart);

    while(1){
        sleep(2);
    }

    

    // if(argc != 3)
    //  errexit("Usage: %s concert console port\n", argv[0]);

    // int** tb = new int*[ROLE_AMO];
    // for (int i = 0;i < ROLE_AMO; i++){
    //     tb[i] = new int [3];
    //     tb[i][0] = i;
    //     tb[i][1] = DEFAULT;
    //     tb[i][2] = 1;
    // }
    // game_server game_server(11, tb);

    // ////// checking vote ////////
    // string str_arr[game_server.alive] = {"--p 0 --vote 2", "--p 1 --vote -1", "--p 2 --vote 1"\
    // , "--p 6 --vote 7", "--p 9 --vote 1", "--p 10 --vote -1", "--p 3 --vote 6"
    // , "--p 4 --vote 7", "--p 7 --vote 4", "--p 8 --vote 7", "--p 5 --vote 9"};
    // game_server.vote_update(str_arr);
    // cout << "\n\nEvent_des : \n\n";
    // cout << game_server.event_des << endl;
    // cout << "\n------------------------------\n";

    // ////// checking night_update ////////
    // game_server.alive ++;
    // string str_arr1[game_server.alive] = {"--role 0 --obj 10", "--role 1 --obj 7", "--role 2 --obj 1"\
    // , "--role 6 --obj 5", "--role 9 --obj -1", "--role 10 --obj 6", "--role 3 --obj 5"\
    // , "--role 4 --obj 5", "--role 7 --obj 5", "--role 8 --obj 10", "--role 5 --obj -2"};
    // game_server.night_update(str_arr1);   
    // for (int i = 0; i < ROLE_AMO ; i++){
    //     cout << i << ": " << game_server.respond[i];
    // }
    // if (game_server.intimidate_obj >= 0){
    //     cout << "Player " << game_server.intimidate_obj << " will be prohibited to chat\n";
    // } 
    // cout << "\n\nEvent_des : \n\n";
    // cout << game_server.event_des << endl;

    // //// check god father alive ////
    // bool check = game_server.godfather_alive_check();
    // if (check){
    //     cout << "god father dead.\n";
    //     cout << "next god father player : " << game_server.role_table[6][0] << endl;
    //     cout << "command :--gf\n";
    // }
    // //// check game over ////
    // check = game_server.game_over_check();

}