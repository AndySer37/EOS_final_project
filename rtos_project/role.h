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

using namespace std;
#define BUFSIZE 1024
#define SERSIZE 30
#define Server_output false

const string role_name[] = {" ", "police", "detective", "bodyguard", "doctor", "spy", "soldier"\
                    , "father", "Intimidate", "streetwalker", "survivor", "killer"};
const string group_name[] = {"Town", "Mafia", "Neutral/Kindness", "Neutral/Evil"};
const string winning_cond[] = {
    "Put all the criminal to Death!", 
    "Kill all the people live in the TOWN and the person who opposite you!",
    "Be alive until the game end",
    "Kill all the people in the game"
};
void *connection_handler(void *);

class Role{
  public:
    // 1 police, 2 detective, 3 bodyguard, 4 doctor, 5 spy, 6 soldier
    // 7 father, 8 Intimidate, 9 streetwalker
    // 10 survivor, 11 killer
    int Role_id;
    int player;
    int player_obj;
    bool using_skill;
    int state_check;            // Day chatting, voting, night, night ending

    int group;
    int ability_count;
    int connfd;
    bool alive;
    bool chating_ability;
    char snd[BUFSIZE], rcv[BUFSIZE];
    Role *cls_ptr;
    pthread_t thread_id;

    Role(){}
    Role(int, int, int);
    ~Role(){}
    void day_func();
    void night_func();
    string input();
    void output(string);
    int vote_period();
    void save_ptr(Role*);
    bool check_obj(string);

};
void Role::save_ptr(Role *p){
    cls_ptr = p;
    // cout <<  " " << cls_ptr << endl;
    if( pthread_create( &thread_id, NULL ,  connection_handler , (void*) cls_ptr) < 0){
        perror("could not create thread");
        return;
    }
    while(1){
        switch(state_check){
            case 0:
                day_func();
                break;
            case 1:
            case 2:
                night_func();
                break;
            //case 3:
        }
    }
}
Role::Role(int con, int role, int player){
    state_check = 2;
    connfd = con;
    Role_id = role;
    this->player = player;
    alive = true;
    chating_ability = true;
    using_skill = false;

    cout << "You are player " << player << endl;
    cout << "Your role is " << role_name[role] << endl;
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
    cout << "Your group is : " << group_name[group] << endl;
    cout << "Your winning condition is : " << winning_cond[group] << endl;  
}
int Role::vote_period(){
    string kb_input = "";
    kb_input = input();
}
void Role::day_func(){
    string sent;
    cout << "============= Day Time =============\n";
    if(chating_ability){
        while(state_check == 0){
            sent = input();
            output(sent);
        }
    }
}
void Role::night_func(){
    cout << "Now, night is coming\n";

    string kb_input = "";
    player_obj = -1;
    using_skill = false;
    char *pch;
    const char *res;
    while(state_check == 2){  
        cout << "Please make a decision\n";
        cout << "Command : --use, --notuse, --obj [player num], [chatting]\n";
        kb_input = input();
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
                    cout << "Your selection is player " << player_obj << endl;
                    break;
                // defense
                case 6:
                case 10:
                    cout << "Your decision is ";
                    if(using_skill)
                        cout << "launch.\n";
                    else
                        cout << "not to launch.\n";
                    break;
                default:
                    break;
            }
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
        using_skill = true;
        return true;
    }    
    if (str == "--notuse"){
        using_skill = false;
        return true;
    }      
    char *c_ptr;
    char *cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    
    c_ptr = strstr (cstr,"--obj "); 
    if (c_ptr != NULL){
        char *c = (c_ptr + 6);
        player_obj = atoi(c);
        return true;
    }
    return false;
}
string Role::input(){
    string kb_input = "";
    char c;
    cin.get(c);
    while(1){   
        if(c == '\n'){
            break;
        }
        kb_input += c;
        cin.get(c);
    } 
    return kb_input;   
}
void Role::output(string out){
    int n;
    if ((n = write(connfd, out.c_str(), strlen(out.c_str()))) == -1)
        errexit("Error: write()\n"); 
}


void *connection_handler(void *conn){
    Role *ptr = (Role*)conn;
    int n;
    char snd[BUFSIZE], rcv[BUFSIZE];
    while(1){
        memset(rcv, 0, BUFSIZE);
        memset(snd, 0, BUFSIZE);
        if((n = read(ptr->connfd, rcv, BUFSIZE)) == -1)
            errexit("Error: read()\n");
        // system("clear");
        cout << rcv << endl;
    }
    pthread_exit((void *)conn);
    return 0;
} 