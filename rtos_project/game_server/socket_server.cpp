#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <csignal> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "game_server.h"

using namespace std;
#define MAX_SOCKET_CONNECTION 80
#define SOCKET_PORT "8787"
#define SOCKET_IP "127.0.0.1"
#define GAME_COUNTDOWN_SECOND 5

typedef struct sockaddr *sockaddrp;
typedef enum{
    GAME_IDLE=0,
    GAME_LOADING,
    GAME_LOGIN,
    GAME_AWARD,
    GAME_COMPUTATION,
    GAME_SHUTDOWN,
    MORNING_CHAT,
    MORNING_VOTE,
    MORNING_EFFECT,
    NIGHT,
}GameState;
volatile GameState game_state;

const char GAME_STATE_MSG[][30] = {
    "GAME_IDLE",
    "GAME_LOADING",
    "GAME_LOGIN",
    "GAME_AWARD",
    "GAME_COMPUTATION",
    "GAME_SERVER_SHUTDOWN",
    "MORNING_CHAT",
    "MORNING_VOTE",
    "MORNING_EFFECT",
    "NIGHT",
};


struct sockaddr_in src_addr[MAX_SOCKET_CONNECTION];    //to record client ip
socklen_t src_len = sizeof(src_addr[0]);
int server_sockfd = 0;                     // file descriptor of socket server 
int confd[MAX_SOCKET_CONNECTION] = {0};    // to record file descriptors of socket
int num_conn = 0;                          // to record how many people are joining in game room
timer_t server_timer;                      // timer object of server
clock_t t_gamestart, t_stamp;
volatile unsigned cnt_systick = 0;         // to count system tick timer

game_server gs;
string *user_effect_list;



int socket_init(int *sockfd){
    // Allocate socket
    *sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(*sockfd < 0){
        perror("socket");
        return -1;
    }

    // Input port and ip from args 
    struct sockaddr_in addr = {AF_INET};
    addr.sin_port = htons(atoi(SOCKET_PORT));    // Convert host btye order to network byte order
    addr.sin_addr.s_addr = inet_addr(SOCKET_IP);
    socklen_t addr_len = sizeof(addr);

    //socket bind
    int ret = bind(*sockfd, (sockaddrp)&addr, addr_len);
    if(ret < 0) {
        perror("bind");
        return -1;
    }

    // Set Max number of socket connection
    listen(*sockfd, MAX_SOCKET_CONNECTION);
}

void socket_broadcast(char const *announcement_type, char const *str){
    int i=-1;
    char buf_snd[100] = {};
    char type_str[20] = {};

    if(strcmp(announcement_type, "SYSTEM") == 0 || strcmp(announcement_type, "system") == 0)
        sprintf(type_str, "\033[0;33m[SYSTEM]\033[0m ");
    else if(strcmp(announcement_type, "ERROR") == 0 || strcmp(announcement_type, "error") == 0)
        sprintf(type_str, "\033[0;31m[SYSTEM]\033[0m ");

    sprintf(buf_snd, "%s%s", type_str, str);
    cout << buf_snd << endl;
    for(int i = 0; i <= num_conn; i++){
        send(confd[i], buf_snd, sizeof(buf_snd), 0);
    }
}

// Shutdown Handler
void shutdown_handler(int signal_num){
    game_state = GAME_SHUTDOWN;
    socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
    for(int i = 0; i <= num_conn; i++)
        close(confd[i]);
    close(server_sockfd);
     
    // terminate program   
    exit(signal_num);   
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
    if(game_state != GAME_LOGIN){
        time(&t_stamp);
        sprintf(buf_snd, "Sorry, the game has been started for %.0lf %s(s). Please wait for next round\n", 
            (difftime(t_stamp, t_gamestart)<60)? difftime(t_stamp, t_gamestart):difftime(t_stamp, t_gamestart)/60,
            (difftime(t_stamp, t_gamestart)<60)? "sec": "min");
        send(confd[userid], buf_snd, strlen(buf_snd), 0);
        confd[userid] = -1;
        pthread_exit(0);
    }else {
        sprintf(buf_snd, "Welcome [%s] to the game", username);
        socket_broadcast("SYSTEM", buf_snd);
    }
    
    

    // A loop to deal with the messages from clients
    while(1){
        bzero(buf_rcv, sizeof(buf_rcv));    // clear rcv buffer
        recv(confd[userid], buf_rcv, sizeof(buf_rcv), 0);

        // User quit
        if(strlen(buf_rcv) == 0/* or strcmp("quit",buf_rcv) == 0*/){
            sprintf(buf_snd, "[%s] quit the game", username);
            socket_broadcast("SYSTEM", buf_snd);
            confd[userid] = -1;
            pthread_exit(0);
        }

        if(game_state == MORNING_CHAT || game_state == GAME_LOGIN){
            // broadcast to each other
            sprintf(buf_snd, "[%s]: %s", username, buf_rcv);
            cout << buf_snd << endl;
            for(int i = 0; i <= num_conn; i++){
                if(i == userid || confd[i] == 0) continue;    // skip the user who send the msg
                send(confd[i], buf_snd, sizeof(buf_snd), 0);
            }
        }
        else if(game_state == MORNING_EFFECT){

        }

        else if(game_state == MORNING_VOTE){

        }

        else if(game_state == NIGHT){

        }
    }
}

void systick_handler(int iSignNo){
    cnt_systick++;
}

int systick_init(timer_t *timer){
    struct sigevent evp;  
    struct itimerspec ts;  
     
    int ret;
    evp.sigev_value.sival_ptr = timer;  
    evp.sigev_notify = SIGEV_SIGNAL;  
    evp.sigev_signo = SIGUSR1;  
    signal(evp.sigev_signo, systick_handler); 
    ret = timer_create(CLOCK_REALTIME, &evp, timer);  
    if(ret) {
        perror("timer_create");
        return -1;
    } 
    ts.it_interval.tv_sec = 1;      // the spacing time  
    ts.it_interval.tv_nsec = 0;  
    ts.it_value.tv_sec = 0;         // the delay time start
    ts.it_value.tv_nsec = 0;
    cout << "SysTick start..." << endl;
    ret = timer_settime(timer, 0, &ts, NULL);  
    if(ret) perror("timer_settime");
}

void *login_handler(void *socketfd){
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

        // Make this process sleep 0.5 sec
        usleep(500000);
    }
}

void wait_timer_countdown(unsigned int sec){
    cnt_systick = 0;
    char buf_snd[100] = {};

    while(cnt_systick < sec){
        if(sec - cnt_systick <= GAME_COUNTDOWN_SECOND){
            sprintf(buf_snd, "%s count down %d sec(s)", GAME_STATE_MSG[game_state], sec - cnt_systick);
            socket_broadcast("", buf_snd);
        }
        cnt_systick++;
        usleep(950000);
    }
}

int main(int argc, char *argv[]){
    pthread_t tid;
    srand(time(NULL));
    if(systick_init(&server_timer) < 0) return -1;
    if(socket_init(&server_sockfd) < 0) return -1;
    // User login handler            
    if(pthread_create(&tid, NULL, login_handler, (void*) &server_sockfd) < 0){
        perror("pthread_create");
        return -1;
    }
    // Shutdown handler
    signal(SIGINT, shutdown_handler); 

    // Game Login
    game_state = GAME_LOGIN;
    socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
    while(num_conn < 3){
        wait_timer_countdown(20);
        if(num_conn >= 3) break;
        socket_broadcast("SYSTEM", "Not enough players, continue waiting...");
    }
    
    // Game Loading
    game_state = GAME_LOADING;
    socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);

    // Create game server object
    int** tb = new int*[ROLE_AMO];
    for (int i = 0;i < ROLE_AMO; i++){
        tb[i] = new int [3];
        tb[i][0] = DEFAULT;     // player
        tb[i][1] = DEFAULT;     // obj
        tb[i][2] = 0;           // alive or not
    }

    int num_players=0, i=0;
    do{
        int role_idx = rand() % ROLE_AMO;
        // if user exit the game || role repeat
        if(confd[i] == -1 || tb[i][role_idx] == DEFAULT) continue;
        tb[role_idx][0] = i;
        tb[role_idx][1] = DEFAULT;
        tb[role_idx][2] = 1;
        num_players++;
    }while(i++ < num_conn);
    gs = game_server(num_players, tb);

    
    for (int i = 0;i < ROLE_AMO; i++){
        char buf_snd[30] = {};
        // if the role exist, send msg to client corresponding to the role.
        if(gs.role_table[i][2] == 1){     
            // sprintf(buf_snd, "--role %d --p %d --amo %d", i, gs.role_table[i][1], num_players);
            // send(confd[gs.role_table[i][1]], buf_snd, sizeof(buf_snd), 0);
        }
    }


    // Game Loop:  MORNING_CHAT --> MORNING_VOTE --> NIGHT --> 
    while(1){
        game_state = MORNING_CHAT;
        socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
        time(&t_gamestart);
        wait_timer_countdown(60);

        game_state = MORNING_VOTE;
        socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
        wait_timer_countdown(30);

        game_state = NIGHT;
        socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
        wait_timer_countdown(45);
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