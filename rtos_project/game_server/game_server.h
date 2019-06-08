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
#define DEFAULT -3
#define NOT_USE -2
#define USED -1

const string DEATH_DES[] = {"was shot to death last night.\n", \
    "was dying from a duel last night.\n", \
    "was involved in a massacre which is made by a crazy soldier.\n"};
const string role_name[] = {" ", "police", "detective", "bodyguard", "doctor", "spy", "Retired soldier"\
                    , "Godfather", "Intimidate", "streetwalker", "survivor", "Serial killer"};
const int Town_list[] = {0,1,2,3,4,5};
const int Mafia_list[] = {6,7,8};
struct death_info{
    int player;
    string death_des;
};
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
    // index role, row0: player, row1: objective -2 for none/ -1 for use/ 0 ~  for player
    // row2: alive or not: 1 / 0
    int** role_table;
    int alive;       // amount of alive in game
    int night;       // night X
    int vote_death;  // if -1: nobody death when voting
    int* vote_arr;      // store the vote // index: player 
    int intimidate_obj;
    string event_des;   // for voting, night ending and game ending
    string* respond;    // for each player's respond for night ending
    vector < death_info > death_list;

    bool night_update(string *);        
    bool vote_update(string *);         // --vote [num] , not voting if [num] == -1 
    bool godfather_alive_check();
    string command;                     // ????
    bool game_over_check();
};

bool game_server::godfather_alive_check(){
    /// if godfather is dead
    if(role_table[6][2] == 0){
        if(role_table[7][2] == 1){          /// godfather and Intimidate switch
            for(int i = 0; i < 3; i++){
                int buf = role_table[6][i];
                role_table[6][i] = role_table[7][i];
                role_table[7][i] = buf;
            }
            return true;
        }
        if(role_table[8][2] == 1){          /// godfather and streetwalker switch 
            for(int i = 0; i < 3; i++){
                int buf = role_table[6][i];
                role_table[6][i] = role_table[8][i];
                role_table[8][i] = buf;
            }
            return true;
        }
    }
    return false;
}
bool game_server::game_over_check(){
    /// check the game status
    int i = 0;
    int town_len = sizeof(Town_list)/sizeof(int);
    int mafia_len = sizeof(Mafia_list)/sizeof(int);
    int town_alive = 0, mafia_alive = 0;
    for(i = 0; i < town_len; i++){
        town_alive += role_table[Town_list[i]][2];
    }
    for(i = 0; i < mafia_len; i++){
        mafia_alive += role_table[Mafia_list[i]][2];
    } 
    // alive = 1
    if (alive == 1){
        if(town_alive)
            event_des = "Town Winning.\n";
        else if(mafia_alive)
            event_des = "Mafia Winning.\n";
        else if(role_table[10][2])
            event_des = "Serial Killer Winning.\n";
        else if(role_table[9][2])
            event_des = "Survivor Winning.\n";
        return true;
    }
    // alive = 2
    if (alive == 2){
        if(town_alive){
            if(mafia_alive)
                event_des = "Mafia Winning.\n";
            else if(role_table[10][2])
                event_des = "Serial Killer Winning.\n";
            else if(role_table[9][2])
                event_des = "Town and Survivor Winning.\n";
            else
                event_des = "Town Winning.\n";
        }
        else if(mafia_alive){
            if(role_table[10][2])
                event_des = "Serial Killer Winning.\n";
            else if(role_table[9][2])
                event_des = "Mafia and Survivor Winning.\n";
            else
                event_des = "Mafia Winning.\n";
        }
        else{
            event_des = "Serial Killer and Survivor Winning.\n";
        }
        return true;
    }
    // alive > 2
    if (town_alive == 0){
        if(role_table[10][2] == 0){
            if(role_table[9][2])
                event_des = "Mafia and Survivor Winning.\n";
            else
                event_des = "Mafia Winning.\n";
            return true;
        }
    }
    if (mafia_alive == 0){
        if(role_table[10][2] == 0){
            if(role_table[9][2])
                event_des = "Town and Survivor Winning.\n";
            else
                event_des = "Town Winning.\n";
            return true;
        }        
    } 
    return false;
}

// 1 police, 2 detective, 3 bodyguard, 4 doctor, 5 spy, 6 soldier
// 7 Godfather, 8 Intimidate, 9 streetwalker
// 10 survivor, 11 Serial killer

game_server::game_server(int alive, int** tb){
    this->alive = alive;
    this->role_table = tb;
    this->night = 0;
    this->respond = new string[ROLE_AMO];
    this->vote_arr = new int[ROLE_AMO];
    this->event_des = "";
}

// Update functions
// --role [num] --obj [obj]
// for alive times

// if obj == -1 used , -2 not used, else objective
bool game_server::night_update(string *str_arr){   
    night += 1; 
    char *pch, *pch1, *obj_player;
    int i;
    string role;    
    ostringstream ss;
    death_list.clear();  /// reset death_list
    ss.str("");
    /// reset role_table /// 
    for (i = 0; i < ROLE_AMO; i++){
        role_table[i][1] = DEFAULT;
        respond[i] = "";
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
        role_table[atoi(role.c_str())][1] = atoi(obj_player);
    }
    //// check role_table ////
    // for (i = 0; i < ROLE_AMO; i++){
    //     cout << role_table[i][0] << " " << role_table[i][1] << endl;
    // }
    int obj;
    //// police ////
    if (role_table[0][1] != DEFAULT && role_table[0][1] != NOT_USE){
        obj = role_table[0][1];
        for (i = 0; i < ROLE_AMO; i++){
            if (role_table[i][0] == obj){
                break;
            }
        }
        switch(i){
            // Town
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 9:
                respond[role_table[0][0]] = "His/Her identity isn't suspicious\n";
                break;
            // Mafia
            case 6:
            case 7:
            case 8:
                respond[role_table[0][0]] = "His/Her identity is Mafia\n";
            // neutrality
            case 10:
                respond[role_table[0][0]] = "His/Her identity is Serial killer\n";
                break;
        }
    }

    //// detective ////
    if (role_table[1][1] != DEFAULT && role_table[1][1] != NOT_USE){
        obj = role_table[1][1];
        for (i = 0; i < ROLE_AMO; i++){
            if (role_table[i][0] == obj){
                break;
            }
        }
        switch(i){
            // Town
            case 1:
            case 4:
            case 6:
            case 10:
                respond[role_table[1][0]] = "Illegal invasion: Maybe he/she is detective, spy, Godfather, Serial killer\n";
                break;
            case 0:
            case 3:
            case 9:
                respond[role_table[1][0]] = "No criminal behavior: Maybe he/she is police, doctor, survivor\n";
                break;
            case 2:
            case 5:
            case 7:
            case 8:
                respond[role_table[1][0]] = "Disturb the peace: Maybe he/she is bodyguard, retired soldier, Intimidate, streetwalker\n";
                break;
        }
    }

    //// streetwalker ////
    if (role_table[8][1] != DEFAULT && role_table[8][1] != NOT_USE){
        obj = role_table[8][1];
        for (i = 0; i < ROLE_AMO; i++){
            if (role_table[i][0] == obj){
                break;
            }            
        }
        if (role_table[2][1] != obj && i != 10){           // streetwalker bodyguard have same obj
            respond[role_table[8][0]] = "You had successfully restricted your objective's action.\n";
            respond[role_table[i][0]] = "Your action had been restricted, you couldn't do anything.\n";
        }
        else if (i == 10){      // Useless for serial killer
            respond[role_table[8][0]] = "You had successfully restricted your objective's action.\n";
        }
    }

    //// bodyguard ////
    bool check = false;
    if (role_table[2][1] != -2 && respond[role_table[2][0]] == ""){ // streetwalker had slept first
        obj = role_table[2][1];
        for (i = 0; i < ROLE_AMO; i++){
            if (role_table[i][1] == obj){
                if (i == 6 || i == 7 || i == 8 || i == 10){
                    if (respond[role_table[i][0]] == ""){       // streetwalker had slept first
                        check = true;
                        break;
                    }
                }
            }            
        }
        if(check){
            respond[role_table[2][0]] = "You involved in a flight !!!\n";
            respond[role_table[i][0]] = "You involved in a flight !!!\n";
            struct death_info a, b;
            a.player = role_table[2][0];
            a.death_des = DEATH_DES[1];
            b.player = role_table[i][0];
            b.death_des = DEATH_DES[1];
            role_table[2][2] = 0;
            role_table[i][2] = 0;
            death_list.push_back(a);
            death_list.push_back(b);
        }
        else{
            respond[role_table[2][0]] = "Your objective was peace tonight.\n";
        }
    }
    //// spy ////
    if (role_table[4][1] != DEFAULT && role_table[4][1] != NOT_USE && respond[role_table[4][0]] == ""){     // streetwalker had slept first
        obj = role_table[4][1];
        for (i = 0; i < ROLE_AMO; i++){
            if (role_table[i][0] == obj){
                break;
            }
        }
        if (role_table[i][1] != -2){
            ostringstream stri;
            stri.str("");
            stri << role_table[i][1];
            respond[role_table[4][0]] = "Your objective had visited player " + stri.str() + ".\n";     
        }
        else{
            respond[role_table[4][0]] = "Your objective had no action.\n";
        }
    }
    //// soldier ////
    if (role_table[5][1] != -2){
        for(i = 0; i < ROLE_AMO; i++){
            if (i != 10 && role_table[i][1] == role_table[5][0] && respond[role_table[i][0]] != "Your action had been restricted, you couldn't do anything.\n"){
                respond[role_table[i][0]] = "You " + DEATH_DES[2];
                struct death_info a;
                a.player = role_table[i][0];
                a.death_des = DEATH_DES[2];
                role_table[i][2] = 0;
                death_list.push_back(a);
            }
        }
        respond[role_table[5][0]] = "You were in Alert State !!\n";
    }
    bool doctor_save = false;
    bool survivor_save = false;
    //// Serial killer ////
    if (role_table[10][1] != DEFAULT && role_table[10][1] != NOT_USE && respond[role_table[10][0]] == ""){     // Maybe killed by bodyguard
        obj = role_table[10][1];

        for(i = 0; i < ROLE_AMO; i++){
            if (role_table[i][0] == obj){
                break;
            } 
        }
        if(i == 9 && role_table[9][1] == USED && respond[role_table[9][0]] == ""){
            survivor_save = true;
        }
        else if (role_table[3][1] == role_table[i][0] && respond[role_table[3][0]] == ""){   // save by doctor
            doctor_save = true;
        }
        else{
            respond[role_table[i][0]] = "You were shoot dead during midnight!\n";
            struct death_info a;
            a.player = role_table[i][0];
            a.death_des = DEATH_DES[0];
            role_table[i][2] = 0;
            death_list.push_back(a);    
        }
        respond[role_table[10][0]] = "You successfully executed the shooting !\n";
    }
    //// Godfather ////
    if (role_table[6][1] != DEFAULT && role_table[6][1] != NOT_USE && respond[role_table[6][0]] == ""){
        obj = role_table[6][1];
        for(i = 0; i < ROLE_AMO; i++){
            if (role_table[i][0] == obj){
                break;
            } 
        }
        if(i == 9 && role_table[9][1] == USED && respond[role_table[9][0]] == ""){
            survivor_save = true;
        }
        else if (role_table[3][1] == role_table[i][0] && respond[role_table[3][0]] == ""){   // save by doctor
            doctor_save = true;
        }
        else{
            respond[role_table[i][0]] = "You were shoot dead during midnight!\n";
            struct death_info a;
            a.player = role_table[i][0];
            a.death_des = DEATH_DES[0];
            role_table[i][2] = 0;
            death_list.push_back(a);    
        }
        respond[role_table[6][0]] = "You successfully executed the shooting !\n";
    }
    //// Intimidate ////
    intimidate_obj = role_table[7][1];
    if (role_table[7][1] != DEFAULT && role_table[7][1] != NOT_USE && respond[role_table[7][0]] == ""){
        respond[role_table[7][0]] = "You had successfully intimidated your objective.\n";
    }
    //// doctor //// 
    if (respond[role_table[3][0]] == ""){
        if (doctor_save){
            respond[role_table[3][0]] = "You had successfully saved a life.\n";
        }
        else{
            respond[role_table[3][0]] = "Your objective still alive or incurable.\n";
        }
    }
    //// survivor ////
    if (respond[role_table[9][0]] == ""){
        if (survivor_save){
            respond[role_table[9][0]] = "You had successfully survived from a shooting.\n";
        }
        else{
            respond[role_table[9][0]] = "Your objective still alive or incurable.\n";
        }
    }
    for (i = 0 ; i < ROLE_AMO ; i++){
        if (respond[role_table[i][0]] == "" && role_table[i][1] == NOT_USE){
            respond[role_table[i][0]] = "You didn't do anything last night.\n";
        }
    }
    vector<struct death_info>::iterator iter = death_list.begin();
    ss << "------------------- night " << night << " -------------------\n";
    if (iter == death_list.end()){
        ss << "Last night was peace, nobody was dead!!\n";
    }
    else{
        for(iter; iter != death_list.end(); ++iter){
            ss << "Player " << iter->player << " " << iter->death_des;
            alive --;
        } 
    }
    event_des = ss.str();
    return true;
}
// --p [num] --vote [vote_player]
// [vote_player] == -1 , not voting
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
    event_des = ss.str();
    return true;
}

