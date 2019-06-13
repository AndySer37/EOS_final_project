#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream> 
#include <vector>
#include <iomanip>
#include <syslog.h>
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdio>
#include <ctime>
#include <ctype.h>
#include <termios.h>

#include "windows.h"
// --role [num] --obj [obj]                  for night: role for Role_id, obj for player or  -2 for none/ -1 for use/ 0 ~  for player
// --p [num] --vote [vote_player]            // [vote_player] == -1 , not voting
// --vote 2
using namespace std;
#define BUFSIZE 1024
#define SERSIZE 30
#define Server_output false
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
const string role_name[] = {" ", "police", "detective", "bodyguard", "doctor", "spy", "Retired soldier"\
                    , "Godfather", "Intimidate", "streetwalker", "survivor", "Serial killer"};
const string group_name[] = {"Town", "Mafia", "Neutral/Kindness", "Neutral/Evil"};
const string winning_cond[] = {
    "Put all the criminal to Death!", 
    "Kill all the people live in the TOWN and the person who opposite you!",
    "Be alive until the game end",
    "Kill all the people in the game"
};

stringstream ss_win;
Windows w;

char buf_snd_role[255] = {0};                // temporary key in buffer
void *connection_handler(void *);
int getch_role(void){
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
class Role{
  public:
    // 1 police, 2 detective, 3 bodyguard, 4 doctor, 5 spy, 6 soldier
    // 7 father, 8 Intimidate, 9 streetwalker
    // 10 survivor, 11 killer
    int Role_id;
    int player;
    int player_obj;
    int state_check;            // Day chatting, voting, night, night ending
    int voting;
    int player_amount;
    int group;
    int ability_count;
    int connfd;
    int n;
    int night;
    // int role;
    bool game_over;
    bool alive;
    bool chating_ability;
    bool *alive_list;
    bool using_skill;

    char snd[BUFSIZE], rcv[BUFSIZE];
    Role *cls_ptr;

    pthread_t thread_id;


    Role(){}
    Role(int, int, int, int);
    ~Role(){}
    void day_func();
    void night_func();
    void output(string);
    int vote_period();
    void save_ptr(Role*);
    bool check_obj(string);
    int check_voting(string);
    void game_start_update(string);

};
void Role::game_start_update(string str){

    stringstream in_ss;
    in_ss << str;
    string n;
    int p;
    char c;
    char line[100];
    char *pch, *pch1 ,*playerp;
    int count = 0;
    while(count != 1){
        in_ss.get(c);
        if(c == '\n')
            count += 1;
    }


    for (int i = 0; i < player_amount; i++){
        in_ss.getline(line,100); 
        n = "";
        char *cstr = new char[string(line).length() + 1];
        strcpy(cstr, line);
        pch = strstr (cstr,"Name: ");
        pch1 = strstr (cstr,"Player: ");
        for (int j = 0; j < (pch1 - pch - 7) ; j++){
            n += *(pch + 6 + j);
        }
        playerp = (pch1 + 8);
        p = atoi(playerp);

        // cout << "N: " << n << " id: " << p << endl;
    }    


}
void Role::save_ptr(Role *p){

    cls_ptr = p;
    // cout <<  " " << cls_ptr << endl;
    // cout << "You are player " << player << endl;
    // cout << "Your role is " << role_name[Role_id] << endl;
    // cout << "Your group is : " << group_name[group] << endl;
    // cout << "Your winning condition is : " << winning_cond[group] << endl;
    ss_win << "You are player " << player << endl;
    w.recv_msg(1, ss_win); 
    ss_win << "Your role is " << role_name[Role_id] << endl;
    w.recv_msg(1, ss_win); 
    ss_win << "Your group is : " << group_name[group] << endl;
    w.recv_msg(1, ss_win); 
    ss_win << "Your winning condition is : " << winning_cond[group] << endl;
    w.recv_msg(1, ss_win); 
    if( pthread_create( &thread_id, NULL ,  connection_handler , (void*) cls_ptr) < 0){
        perror("could not create thread");
        return;
    }
    while(alive && !game_over){
        switch(state_check){
            case 0:
                day_func();
                break;
            case 1:
                vote_period();
                break;
            case 2:
                night_func();
                break;
            case 3:
                day_func();
                break;                
        }
    }
    if (!alive){
        chating_ability = true;
        // cout << "You are dead !!\n";
        ss_win << "You are dead !!\nNow you can chat with other death.";
        w.recv_msg(1, ss_win);
    }
    while(!game_over){
        day_func();
    }
    ss_win.str("");
    ss_win << "Client Close.";
    w.recv_msg(1, ss_win);
}
Role::Role(int con, int role, int player, int player_amount){
    this->night = 0;
    this->state_check = 0;
    this->connfd = con;
    this->Role_id = role + 1;
    this->player = player;
    this->player_amount = player_amount;
    this->alive = true;
    this->chating_ability = true;
    this->using_skill = false;
    this->game_over = false;
    this->alive_list = new bool(player_amount);
    for (int i = 0 ; i < player_amount ; i++){
        alive_list[i] = true;
    }

    switch(Role_id){
        // Town
        case 6:
            ability_count = 2;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            group = 0;
            break;
        // Mafia
        case 7:
        case 8:
        case 9:
            group = 1;
        // neutrality
        case 10:
            group = 2;
            ability_count = 4;
            break;
        case 11:
            group = 3;
            break;
        default:
            ability_count = 0;
    }

}
int Role::vote_period(){
    string kb_input = "";
    while(state_check == 1){  
        // cout << "Please make a decision\n";
        // cout << "Command : --vote [player num]\n";
        // cout << "not to vote if [player num] = -1\n";
        ss_win << "Please make a decision\n";
        w.recv_msg(1, ss_win);
        ss_win << "Command : --vote [player num]\n";
        w.recv_msg(1, ss_win);
        ss_win << "not to vote if [player num] = -1\n";
        w.recv_msg(1, ss_win);
        kb_input = w.input();
        voting = check_voting(kb_input);
        ostringstream ss;
        if(voting != -1){
            ss << "--p " << player << " --vote " << voting;
            if ((n = write(connfd, ss.str().c_str(), strlen(ss.str().c_str()))) == -1)
                errexit("Error: write()\n");   
            // cout << "You decide to vote player " << voting << endl;
            ss_win << "You decide to vote player " << voting << endl;
            w.recv_msg(1, ss_win);
        }
        else{
            ss_win << "--p " << player << " --vote " << voting;
            if ((n = write(connfd, ss.str().c_str(), strlen(ss.str().c_str()))) == -1)
                errexit("Error: write()\n");  
            // cout << "You don't want to vote anyone.\n";
            ss_win << "You don't want to vote anyone.\n";
            w.recv_msg(1, ss_win);
        }
    }
}
void Role::day_func(){
    string sent;
    // cout << "============= Day Time =============\n";
    if(alive)
        ss_win << "============= Day Time =============\n";
    else
        ss_win << "============= Death Zone =============\n";
    w.recv_msg(1, ss_win);
    while(state_check == 0){
        sent = w.input();
        if(chating_ability){
            output(sent);
        }
    }
    
}
void Role::night_func(){
    // cout << "Now, night is coming\n";
    ss_win << "Now, night is coming\n";
    w.recv_msg(1, ss_win);
    string kb_input = "";
    player_obj = -2;
    using_skill = false;
    char *pch;
    const char *res;
    while(state_check == 2){ 
        // cout << "Please make a decision\n";
        // cout << "Command : --use, --notuse, --obj [player num], [chatting]\n";
        ss_win << "Please make a decision\n";
        w.recv_msg(1, ss_win);
        ss_win << "Command : --use, --notuse, --obj [player num], [chatting]\n";
        w.recv_msg(1, ss_win);
        kb_input = w.input();
        using_skill = check_obj(kb_input);
        if(using_skill){
            switch(Role_id){
                // Have obj
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 7:
                case 8:
                case 9:
                case 11:
                    // cout << "Your selection is player " << player_obj << endl;
                    ss_win << "Your selection is player " << player_obj << endl;
                    w.recv_msg(1, ss_win);
                    break;
                // defense
                case 6:
                case 10:
                    // cout << "Your decision is ";
                    ss_win << "Your decision is ";
                    w.recv_msg(1, ss_win);
                    if(player_obj == -1){
                        // cout << "launch.\n";
                        ss_win << "launch.\n";
                        w.recv_msg(1, ss_win);
                    }
                    else if(player_obj == -2){
                        // cout << "not to launch.\n";
                        ss_win << "not to launch.\n";
                        w.recv_msg(1, ss_win);
                    }
                    break;
                default:
                    break;
            }
            ostringstream ss;
            ss_win << "--role " << (Role_id - 1) << " --obj " << player_obj;
            w.recv_msg(1, ss_win);
            if ((n = write(connfd, ss.str().c_str(), strlen(ss.str().c_str()))) == -1)
                errexit("Error: write()\n");  
        }
        else{
            if(group == 1){
                output(kb_input);
            }
        }
    }
}
/// --use, --notuse --obj ? else chatting
bool Role::check_obj(string str){
    if (str == "--use"){
        player_obj = -1;
        return true;
    }    
    if (str == "--notuse"){
        player_obj = -2;
        return true;
    }      
    char *cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());

    if (strstr (cstr,"--obj ")){
        sscanf(cstr, "--obj %d", &player_obj);
        if(player_obj < player_amount && player_obj >= 0){
            if(alive_list[player_obj] && player_obj != player){
                return true;
            }
            else{
                // cout << "Error: Can't choose yourself or dead\n";
                ss_win << "Error: Can't choose yourself or dead\n";
                w.recv_msg(1, ss_win);
            }
        }
        else{
            // cout << "Error: Invaild player number\n";
            ss_win << "Error: Invaild player number\n";
            w.recv_msg(1, ss_win);
        }
    }
    player_obj = -2;
    return false;
}
int Role::check_voting(string str){
    char *cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    int num;

    if (strstr (cstr,"--vote ")){
        sscanf(cstr, "--vote %d", &num);
        if(num == -1){
            return -1;
        }
        if(num < player_amount && num >= 0){
            if(alive_list[num] && num != player){
                return num;
            }
            else{
                // cout << "Error: Can't choose yourself or dead\n";
                ss_win << "Error: Can't choose yourself or dead\n";
                w.recv_msg(1, ss_win);
            }
        }
        else{
            // cout << "Error: Invaild player number\n";
            ss_win << "Error: Invaild player number\n";
            w.recv_msg(1, ss_win);
        }
    }
    return -1;
}

void Role::output(string out){
    int n;
    if ((n = write(connfd, out.c_str(), strlen(out.c_str()))) == -1)
        errexit("Error: write()\n"); 
}

// thread for reading from server
void *connection_handler(void *conn){
    Role *ptr = (Role*)conn;
    int n;
    char *c_ptr;
    char rcv[BUFSIZE];
    int death, _role;
    stringstream ss_win_thread;
    while(1){
        memset(rcv, 0, BUFSIZE);
        if((n = read(ptr->connfd, rcv, BUFSIZE)) == -1)
            errexit("Error: read()\n");
        ///////////////////// 

        if(strstr(rcv, "MORNING_EFFECT")){
            if(death == ptr->player){
                ptr->chating_ability = true;
            }
            else{
                ptr->chating_ability = false;
            }
            ptr->state_check = 3;
        }
        else if(strstr(rcv, "MORNING_CHAT")){
            ptr->state_check = 0;         
        }
        else if(strstr(rcv, "NIGHT")){
            if(!(ptr->chating_ability)){
                ptr->chating_ability = true;
            }
            ptr->state_check = 2;      
        }
        else if(strstr(rcv, "MORNING_VOTE")){
            ptr->state_check = 1;
        }

        if(strstr(rcv, "--true-role")){ 
            sscanf(rcv, "--death %d --true-role %d", &death, &_role);
            ptr->alive_list[death] = false;
            if (death == ptr->player){
                ptr->alive = false;
            }
        }    
        else if(strstr(rcv, "--death ")){ 
            // cout << "SOME ONE IS DEAD======================================\n";
            // ss_win << "SOME ONE IS DEAD======================================\n";
            // w.recv_msg(ss_win);
            int death;
            sscanf(rcv, "--death %d", &death);
            ptr->alive_list[death] = false;
            // cout << death << " " << ptr->player << endl;
            // ss_win << death << " " << ptr->player << endl;
            // w.recv_msg(ss_win);
            if (death == ptr->player){
                // cout << "YOU FUCKING DEAD\n";
                // ss_win << "YOU FUCKING DEAD\n";
                // w.recv_msg(ss_win);
                ptr->alive = false;
            }
        }
        if (strstr(rcv, "--gf")){
            ss_win_thread.str("");
            ss_win_thread << "God Father is dead, So you switch to Godfather.\nNow you can kill a player at night.";
            w.recv_msg(1, ss_win_thread); 
            ptr->Role_id = 7;       
        } 
        if (strstr(rcv, "--game-over")){
            ss_win_thread.str("");
            ss_win_thread << "The Game is Over.";
            w.recv_msg(1, ss_win_thread); 
            ptr->game_over = true;  
            break; 
        } 
        if (strstr(rcv, "--quiet")){
            ss_win_thread.str("");
            ss_win_thread << "You are threatened!!\nCan't talk at morning!!";
            w.recv_msg(1, ss_win_thread); 
            ptr->chating_ability = false;
        } 

        /////////////////////
        // system("clear");

        // cout << rcv << endl;
        ss_win << rcv << endl;
        if(ss_win.str()[0] == '['){
            w.recv_msg(1, ss_win);
        }
        else{
            w.recv_msg(1, ss_win);
        }
        if (strlen(buf_snd_role) != 0){
            // cout << buf_snd_role;
            // ss_win << buf_snd_role;
            // w.recv_msg(ss_win);
        }

    }
    pthread_exit((void *)conn);
    return 0;
} 