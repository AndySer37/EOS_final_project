#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream> 
#include <vector>
#include <iomanip>
#include "sockop.c"
#include <syslog.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>
#include <ctime>

using namespace std;
#define BUFSIZE 1024
#define SERSIZE 30
#define Server_output false
#define ROLE_AMO 11
#define NOT_USE -2
#define USED -1

class game_server{
  public:
    game_server(){}
    game_server(int , int**);
    ~game_server(){
        for(int i = 0; i < ROLE_AMO; i++) {
            delete [] role_table[i];
        }
        delete [] role_table;
        delete [] vote_arr;
        delete [] respond;
    }
    // table size: ROLE_AMO*2
    // index role, row0: player, row2: objective -2 for none/ -1 for use/ 0 ~  for player
    int** role_table;
    int alive;       // amount of alive in game
    int night;       // night X
    int vote_death;  // if -1: nobody death when voting
    int* vote_arr;      // store the vote // index: player 
    string event_des;   // for voting and night ending
    string* respond;    // for each player's respond for night ending

    bool night_update(string *);
    bool vote_update(string *);

};

// 1 police, 2 detective, 3 bodyguard, 4 doctor, 5 spy, 6 soldier
// 7 father, 8 Intimidate, 9 streetwalker
// 10 survivor, 11 killer

game_server::game_server(int alive, int** tb){
    this->alive = alive;
    this->role_table = tb;
    this->night = 0;
    this->respond = new string[ROLE_AMO];
    this->vote_arr = new int[ROLE_AMO];
    this->event_des = "";
    // for (int i = 0; i < ROLE_AMO ; i++){
    //     cout << role_table[i][0] << " " << role_table[i][1] << endl;
    // }

    string str_arr[alive] = {"--p 0 --vote 2", "--p 1 --vote -1", "--p 2 --vote 1"\
, "--p 6 --vote 7", "--p 9 --vote 1", "--p 10 --vote -1", "--p 3 --vote 6"
, "--p 4 --vote 7", "--p 7 --vote 4", "--p 8 --vote 7", "--p 5 --vote 9"};
    vote_update(str_arr);
    this->alive ++;
    string str_arr1[alive] = {"--role 0 --obj 2", "--role 1 --obj -1", "--role 2 --obj 1"\
, "--role 6 --obj 7", "--role 9 --obj 1", "--role 10 --obj -1", "--role 3 --obj 6"
, "--role 4 --obj 7", "--role 7 --obj 4", "--role 8 --obj 7", "--role 5 --obj 10"};
    night_update(str_arr1);   
}

// Update functions
// --role [num] --obj [obj]
// for alive times

bool game_server::night_update(string *str_arr){    
    char *pch, *pch1, *obj_player;
    int i;
    string role;    
    ostringstream ss;
    ss.str("");
    for (i = 0; i < ROLE_AMO; i++){
        role_table[i][1] = NOT_USE;
    }
    for (i = 0; i < alive; i++){
        role = "";
        char *cstr = new char[str_arr[i].length() + 1];
        strcpy(cstr, str_arr[i].c_str());
        pch = strstr (cstr,"--role ");
        pch1 = strstr (cstr,"--obj ");
        for (int j = 0; j < (pch1 - pch - 8) ; j++){
            role += *(pch + 7 + j);
        }
        obj_player = (pch1 + 6);
        cout << "---" << role << " " << obj_player << endl;
        if (atoi(obj_player) != NOT_USE){
            role_table[atoi(role.c_str())][1] = atoi(obj_player);
        }
    }

    for (i = 0; i < ROLE_AMO; i++){
        cout << role_table[i][0] << " " << role_table[i][1] << endl;
    }

    return true;
}
// --p [num] --vote [vote_player]
bool game_server::vote_update(string *str_arr){
    char *pch, *pch1, *vote_player;
    string player;
    int i, max = 0;
    bool same_check = false;
    vote_death = -1;
    ostringstream ss;
    ss.str("");
    ss << "The voting sheet:\n";
    for (i = 0; i < ROLE_AMO; i++){
        vote_arr[i] = 0;
    }
    // if((sizeof(*str_arr)/sizeof(str_arr[0])) != alive){
    //     cout << "Server Error: length of input array ins't equal to alive\n";
    //     return false;
    // }
    for (i = 0; i < alive; i++){
        player = "";
        char *cstr = new char[str_arr[i].length() + 1];
        strcpy(cstr, str_arr[i].c_str());
        pch = strstr (cstr,"--p ");
        pch1 = strstr (cstr,"--vote ");
        for (int j = 0; j < (pch1 - pch - 5) ; j++){
            player += *(pch + 4 + j);
        }
        ss << "player " << player;
        vote_player = (pch1 + 7);
        if (atoi(vote_player) != -1){
            ss << " vote player " << vote_player << endl;
            vote_arr[atoi(vote_player)] ++;
        }
        else{
            ss << " doesn't voting\n";
        }
    }
    for(i = 1; i < ROLE_AMO; i++){
        if (vote_arr[i] > vote_arr[max]){
            max = i;
            same_check = false;
        }
        else if (vote_arr[i] == vote_arr[max]){
            same_check = true;
        } 
    }    
    if(same_check){
        ss << "Nobody is put to death.\n";
    }
    else{
        ss << "Player " << max << " is put to death\n";
        alive --;
    }
    vote_death = max;
    cout << ss.str();
    event_des = ss.str();

}

