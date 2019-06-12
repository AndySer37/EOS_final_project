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
#include <algorithm>

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

struct PlayerTable{
    int alive;
    char username[30];
    int vote_target;
    int obj_target;
};
struct PlayerTable player_tb[MAX_SOCKET_CONNECTION];


struct sockaddr_in src_addr[MAX_SOCKET_CONNECTION];    //to record client ip
socklen_t src_len = sizeof(src_addr[0]);
int server_sockfd = 0;                     // file descriptor of socket server 
int confd[MAX_SOCKET_CONNECTION] = {0};    // to record file descriptors of socket
int num_conn = 0;                          // to record how many people are joining in game room
int num_players = 0;
timer_t server_timer;                      // timer object of server
clock_t t_gamestart, t_stamp;
volatile unsigned cnt_systick = 0;         // to count system tick timer

game_server *gs;
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
    char buf_snd[255] = {};
    char type_str[20] = {};

    if(strcmp(announcement_type, "SYSTEM") == 0 || strcmp(announcement_type, "system") == 0)
        sprintf(type_str, "\033[0;33m[SYSTEM]\033[0m ");
    else if(strcmp(announcement_type, "ERROR") == 0 || strcmp(announcement_type, "error") == 0)
        sprintf(type_str, "\033[0;31m[SYSTEM]\033[0m ");
    else if(strcmp(announcement_type, "INFO") == 0 || strcmp(announcement_type, "info") == 0)
        sprintf(type_str, "[INFO] ");

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
    timer_delete(server_timer); 
     
    // terminate program   
    exit(signal_num);   
}

void *socket_rcv_handler(void *indexp){
    int userid = *(int *)indexp;
    char buf_rcv[255] = {};
    char buf_snd[255] = {};
    char username[30] = {};

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
        sprintf(buf_snd, "Welcome [%s] to the game id=%d", username, userid);
        socket_broadcast("SYSTEM", buf_snd);
        num_players++;
        player_tb[userid].alive = 1;

        sprintf(player_tb[userid].username, "%s", username);
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
            player_tb[userid].alive = 0;
            num_players--; 
            if(gs){
                for(int i=0; i<ROLE_AMO; i++){
                    if(gs->role_table[i][0] == userid){
                        gs->role_table[i][2] = 0;
                        printf("Clear alive item in role_table.\n");
                        break;
                    }
                }
            }

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
        }else if(game_state == MORNING_VOTE){
            int target= -1;
            std::string tmp(buf_rcv);
            // printf("len=%d, %s\n", (int)strlen(buf_rcv), buf_rcv);
            std::size_t found = tmp.find("vote");
            sscanf(&buf_rcv[found], "vote %d", &target);
            printf("p=%d, target=%d\n", userid, target);
            player_tb[userid].vote_target = target;

        }else if(game_state == NIGHT){
            int target= -1;
            std::string tmp(buf_rcv);
            // printf("len=%d, %s\n", (int)strlen(buf_rcv), buf_rcv);
            std::size_t found = tmp.find("obj");
            sscanf(&buf_rcv[found], "obj %d", &target);
            printf("p=%d, target=%d\n", userid, target);
            player_tb[userid].obj_target = target;
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

void update_tb_from_gs(void){

}

// translate voting result or effect result to string array
void table_to_string(char const *type, string *str_arr, int arr_size){
    if(strcmp(type, "VOTE") == 0 || strcmp(type, "vote") == 0){
        int idx=0;
        for(int i=0; i < num_conn; i++){
            if(player_tb[i].alive){
                char buf_vote[20];
                sprintf(buf_vote, "--p %d --vote %d", i, player_tb[i].vote_target);
                str_arr[idx++] = string(buf_vote);
            }
        }
    }else if(strcmp(type, "EFFECT") == 0 || strcmp(type, "effect") == 0){
        int idx=0; 
        for(int i=0; i < num_conn; i++){
            if(player_tb[i].alive){
                char buf_obj[20];
                sprintf(buf_obj, "--p %d --obj %d", i, player_tb[i].obj_target);
                str_arr[idx++] = string(buf_obj);
            }
        }
    }
    for(int i=0; i<arr_size;i++)
        printf("%s\n", str_arr[i].c_str());
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
    char buf_snd[255] = {};

    while(cnt_systick < sec){
        if(sec - cnt_systick <= GAME_COUNTDOWN_SECOND){
            string tmp=GAME_STATE_MSG[game_state];
            transform(tmp.begin(),tmp.end(),tmp.begin(),::tolower);
            sprintf(buf_snd, "%s count down %d sec(s)", tmp.c_str(), sec - cnt_systick);
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
    while(num_players < 3){
        wait_timer_countdown(10);
        if(num_players >= 3) break;
        socket_broadcast("SYSTEM", "Not enough players, continue waiting...");
    }
    
    // Game Loading
    game_state = GAME_LOADING;
    socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);

    // Create role table and assign the roles randomly
    int** tb = new int*[ROLE_AMO];
    int i=0;
    for (int i = 0;i < ROLE_AMO; i++){
        tb[i] = new int [3];
        tb[i][0] = DEFAULT;     // player
        tb[i][1] = DEFAULT;     // obj
        tb[i][2] = 0;           // alive or not
    }
    for(int i=0; i < num_conn; i++){
        int role_idx;
        if(player_tb[i].alive){
            do{
                role_idx = rand() % ROLE_AMO;
            }while(tb[role_idx][0] != DEFAULT);
            tb[role_idx][0] = i;
            tb[role_idx][1] = DEFAULT;
            tb[role_idx][2] = 1;   
        }
    }
    gs = new game_server(num_players, tb);


    // Send player info to each other
    char buf_snd[300] = {0};
    sprintf(buf_snd, "Players infomation\n");
    printf("num= %d\n", num_conn);
    for(int i=0; i < num_conn; i++){
        if(player_tb[i].alive)
            sprintf(buf_snd, "%sName: %s\tPlayer: %d\n", buf_snd, player_tb[i].username, i);
    }
    socket_broadcast("INFO", buf_snd);
    
    // Send personal role info to corresponding player
    for (int i = 0;i < ROLE_AMO; i++){
        char buf_snd[30] = {};
        if(player_tb[gs->role_table[i][0]].alive && gs->role_table[i][0] != DEFAULT){
            sprintf(buf_snd, "--role %d --p %d --amo %d", i, gs->role_table[i][0], num_players);
            send(confd[gs->role_table[i][0]], buf_snd, sizeof(buf_snd), 0);
            printf("%s\n", buf_snd);
        }
    }


    // Game Loop:  MORNING_CHAT --> MORNING_VOTE --> NIGHT --> 
    while(1){
        game_state = MORNING_CHAT;
        socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
        time(&t_gamestart);
        wait_timer_countdown(3);

        game_state = MORNING_VOTE;
        for(int i=0; i < MAX_SOCKET_CONNECTION; i++)
            player_tb[i].vote_target = -1;
        socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
        wait_timer_countdown(30);

        // Collect and translate the result of voting and input to game_server object
        string str_arr1[gs->alive];
        table_to_string("VOTE", str_arr1, gs->alive);
        gs->vote_update(str_arr1);
        cout << "\n\nEvent_des : \n\n";
        cout << gs->event_des << endl;
        cout << "------------------------------\n";
        // 投票結果宣佈
        if(gs->vote_death != -1){
            char tmp_buf[20];
            sprintf(tmp_buf, "--death %d", gs->vote_death);
            socket_broadcast("", tmp_buf);
            player_tb[gs->vote_death].alive = 0;    //update player tb
        }


        game_state = NIGHT;
        for(int i=0; i < MAX_SOCKET_CONNECTION; i++)    // clear obj table
            player_tb[i].obj_target = -1;
        socket_broadcast("SYSTEM", GAME_STATE_MSG[game_state]);
        wait_timer_countdown(45);
        bool check = gs->game_over_check();

        // Collect and translate the result of effect and input to game_server object
        string str_arr2[gs->alive];
        cout << "After voting gamer : " << gs->alive << endl;
        table_to_string("EFFECT", str_arr2, gs->alive);
        gs->night_update(str_arr2);   
        for (int i = 0; i < ROLE_AMO ; i++)
            cout << i << ": " << gs->respond[i];
        if (gs->intimidate_obj >= 0)
            cout << "Player " << gs->intimidate_obj << " will be prohibited to chat\n";
        cout << "\n\nEvent_des : \n\n";
        cout << gs->event_des << endl;

        // 暗殺結果公佈
        if(gs->death_list.size() > 0){
            for(int i=0; i < gs->death_list.size(); i++){
                char tmp_buf[40];
                gs->death_list[i].player; //亡人
                int true_role, j;
                for(j=0; j < ROLE_AMO; j++)
                    if(gs->role_table[j][0] == gs->death_list[i].player);{
                        true_role = j; //亡人職業
                        break;
                    }
                sprintf(tmp_buf, "--death %d --true-role %d", gs->death_list[i].player, true_role);
                socket_broadcast("", tmp_buf);
                player_tb[gs->death_list[i].player].alive = 0; 
            }
        }

        //// check god father alive ////
        check = gs->godfather_alive_check();
        if (check){
            cout << "god father dead.\n";
            cout << "next god father player : " << gs->role_table[6][0] << endl;
            cout << "command :--gf\n";
        }
        //// check game over ////
        check = gs->game_over_check();
    }
    return 0;
}
