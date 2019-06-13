#ifndef _WINDOWS_H_
#define _WINDOWS_H_
#include <iostream>
#include <sstream>
#include <ncurses.h>
#include <string>
#include <unistd.h>
#include <time.h>
using namespace std;

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
const string role_name[] = {"???", "police", "detective", "bodyguard", "doctor", "spy", "Retired soldier"\
                    , "Godfather", "Intimidate", "streetwalker", "survivor", "Serial killer"};
const string group_name[] = {"Town", "Mafia", "Neutral/Kindness", "Neutral/Evil"};
const string winning_cond[] = {
    "Put all the criminal to Death!", 
    "Kill all the people live in the TOWN and the person who opposite you!",
    "Be alive until the game end",
    "Kill all the people in the game"
};
const string role_func[] = {
    "You can survey identity of a person at night either suspicious or not.", 
    "You can survey identity of a person according to the character.",
    "You can protect a person at night.",
    "You can rescue a person who is on the brink of death.",
    "You can track a person at night.",
    "You have totally twice chance to be on the alert at night\nand you will kill all the person who visit you instead of serial killer.",
    "You can kill a person at night.",
    "You can restrict a person not to talk at morning.",
    "You can restrict a person's action at night.",
    "You have four bulletproof vest which can protect you from death.",
    "You can kill a person at night."
};

int kbhit()
{
    int ch = getch();

    if (ch != ERR) {
        ungetch(ch);
        return 1;
    } else {
        return 0;
    }
}

class Player_info
{
public:
    Player_info(){
        is_playing = false;
        is_alive = true;
        name = "";
        id = role_id = 0;
    }
    int id;
    int role_id;
    bool is_alive;
    bool is_playing;
    string name;
};

class Windows
{
public:
    Windows();
    ~Windows();
    void initial();
    void player_info_refresh(int ,int, int, bool );
    void output_refresh(int ,string );
    void playerlist_refresh();
    void lobby_refresh(int , string );
    void setname(string );
    void msg_filter(int , string );
    void recv_msg(int , stringstream & );
    bool is_player_alive(int i){ return p[i].is_alive; }
    string input();
    string char_intput(int );

private:
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    WINDOW *chat_o, *chat_i, *plist, *sysmsg, *lobby;
    WINDOW *chat_o_box, *chat_i_box, *sysmsg_box;
    
    bool is_plist_show = false;
    bool is_gmae_start = false;
    bool is_name_set = false;

    string msg;
    Player_info p[11], *p_self;
};

Windows::Windows(){
    pthread_mutex_init (&mutex, NULL);

    initial();
    plist=newwin(16,30,(LINES-16)/2,(COLS-30)/2);
    nodelay(plist,TRUE);
    box(plist,'|','-');
    playerlist_refresh();

    sysmsg=newwin(15,78,(LINES)/2-20,(COLS-78)/2);
    sysmsg_box=newwin(17,82,(LINES)/2-21,(COLS-82)/2);
    nodelay(sysmsg,TRUE);
    box(sysmsg_box,'*','*');
    // mvwprintw(sysmsg,2,2,"output\n");
    scrollok(sysmsg,TRUE); 
    touchwin(sysmsg_box);
    wrefresh(sysmsg_box);
    touchwin(sysmsg);
    wrefresh(sysmsg);

    chat_o=newwin(15,78,(LINES)/2-3,(COLS-78)/2);
    chat_o_box=newwin(17,82,(LINES)/2-4,(COLS-82)/2);
    nodelay(chat_o,TRUE);
    box(chat_o_box,'|','-');
    scrollok(chat_o,TRUE); 
    touchwin(chat_o_box);
    wrefresh(chat_o_box);
    touchwin(chat_o);
    wrefresh(chat_o);

    lobby=newwin(15,78,(LINES)/2-3,(COLS-78)/2);
    nodelay(lobby,TRUE);
    wattron(lobby,COLOR_PAIR(7)|A_BOLD);
    mvwprintw(lobby,2,34,"Game Lobby");
    touchwin(lobby);
    wrefresh(lobby);

    chat_i=newwin(5,78,(LINES)/2+14,(COLS-78)/2);
    chat_i_box=newwin(7,82,(LINES)/2+13,(COLS-82)/2);
    noecho();
    nodelay(chat_i,TRUE);
    intrflush(chat_i,FALSE);
    keypad(chat_i,TRUE);
    nodelay(chat_i,TRUE);
    box(chat_i_box,'|','-');
    mvwprintw(chat_i,2,0,"Enter your name :");
    scrollok(chat_i,TRUE);
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
};


void Windows::initial(){
    initscr();
    cbreak();       //unblocking keyboard intupt
    start_color();
    init_color(COLOR_BLACK, 100, 100, 100);
    init_pair(0,COLOR_BLACK,COLOR_BLACK);
    init_pair(1,COLOR_RED,COLOR_BLACK);
    init_pair(2,COLOR_GREEN,COLOR_BLACK);
    init_pair(3,COLOR_YELLOW,COLOR_BLACK);
    init_pair(4,COLOR_BLUE,COLOR_BLACK);
    init_pair(5,COLOR_MAGENTA,COLOR_BLACK);
    init_pair(6,COLOR_CYAN,COLOR_BLACK);
    init_pair(7,COLOR_WHITE,COLOR_BLACK);
    curs_set(0);
    nonl();
    nodelay(stdscr,TRUE);
    noecho();
    // echo();
    intrflush(stdscr,FALSE);
    keypad(stdscr,TRUE);
    wclear(stdscr);
    refresh();
}
void Windows::player_info_refresh(int flag, int i, int role, bool alive){
    if(flag == 0){
        p[i].role_id = role;
    }
    else if(flag == 1){
        p[i].is_alive = alive;
    }
}

void Windows::playerlist_refresh(){
    if(is_plist_show && is_gmae_start){
        wattron(plist,COLOR_PAIR(7)|A_BOLD);
        mvwprintw(plist,2,4,"|  I D  |   role   |");
        for(int i=0,j=0;j<11;j++){
            if(p[j].is_playing){
                if(p[j].is_alive){
                    wattron(plist,COLOR_PAIR(7)|A_BOLD);
                }
                else{
                    wattron(plist,COLOR_PAIR(6));
                }
                mvwprintw(plist,3+i, 2," %d ",i+1);
                mvwprintw(plist,3+i, 5," %-8s ", p[j].name.c_str());
                mvwprintw(plist,3+i,18,role_name[p[j].role_id].c_str());
                i++;
            }
        
        }
        touchwin(plist);
        wrefresh(plist);
    }
    else{
        touchwin(sysmsg_box);
        wrefresh(sysmsg_box);
        touchwin(sysmsg);
        wrefresh(sysmsg);
        touchwin(chat_o_box);
        wrefresh(chat_o_box);
        if(is_gmae_start){
            touchwin(chat_o);
            wrefresh(chat_o);
        }
    }
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
}

void Windows::lobby_refresh(int i, string n){
    p[i].id = i;
    p[i].name = n;
    p[i].is_playing = true;
    if(!is_gmae_start){
        wattron(plist,COLOR_PAIR(7)|A_BOLD);
        mvwprintw(plist,1,0,"|  I D  |             name             |");
        // for(int i=0,j=0;j<11;j++){
        //     mvwprintw(plist,2+i, 2," %d ",i+1);
        //     mvwprintw(plist,2+i, 5," %-8s ", p[j].name.c_str());
        //     i++;
        // }
    }
}

void Windows::output_refresh(int w, string str){
    if(str.empty()){

    }
    else{
        if(w == 0){
            int x, y;
            touchwin(chat_o_box);
            wrefresh(chat_o_box);
            touchwin(chat_o);
            getyx(chat_o, y, x);
            // str +="\n";
            mvwprintw(chat_o, y, 0, str.c_str());
            wrefresh(chat_o);
            touchwin(chat_i_box);
            wrefresh(chat_i_box);
            touchwin(chat_i);
            wrefresh(chat_i);
        }
        else if(w == 1){
            int x, y;
            touchwin(sysmsg_box);
            wrefresh(sysmsg_box);
            touchwin(sysmsg);
            getyx(sysmsg, y, x);
            if(str.substr(0,8) == "(SYSTEM)"){
                wattron(sysmsg,COLOR_PAIR(3)|A_BOLD);
            }
            else{
                wattron(sysmsg,COLOR_PAIR(7));
            }
            // str +="\n";
            mvwprintw(sysmsg, y, 0, str.c_str());
            if(str.substr(0,8) == "(SYSTEM)"){
                wattroff(sysmsg,COLOR_PAIR(3)|A_BOLD);
            }
            wrefresh(sysmsg);
            touchwin(chat_i_box);
            wrefresh(chat_i_box);
            touchwin(chat_i);
            wrefresh(chat_i);
        }
        playerlist_refresh();
    }   
}


string Windows::input(){
    int x;
    msg.clear();
    while(1){
        if (kbhit()) {
            x=getch();
            if(x == 27){
                touchwin(stdscr);
                endwin();
                return "";
            }
            else if(x == '\r'){
                return char_intput(x);
            }
            else if(x == '\t'){
                is_plist_show = !is_plist_show;
                playerlist_refresh();
            }
            else{
                char_intput(x);
            }
        }
    }
}

string Windows::char_intput(int c){
    int x, y;
    touchwin(chat_i);
    getyx(chat_i,y,x);
    if(c == 263){
        if(!msg.empty()){
            mvwprintw(chat_i, y,--x," ");
            wmove(chat_i, y, x);
            msg.pop_back();
        }
    }
    else if(c == '\r'){
        wclear(chat_i);
        if(is_name_set){
            mvwprintw(chat_i,2,0,"Say :");
        }
        else{
            setname(msg);
            is_name_set = true;
        }
    }
    else{
        wprintw(chat_i, "%c", c);
        msg += c;
    }
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
    return msg;
}

void Windows::setname(string msgs){
    is_gmae_start = true;
}
void Windows::msg_filter(int w, string str){
    string first_flag( str.substr(0,str.find_first_of(')',0)) );
    string msg; 
    if(first_flag == "(s)"){
        msg = str.substr(first_flag.size(), -1);
    }
    else if(first_flag == "(p)"){
        msg = str.substr(first_flag.size(), -1);
    }
    else{
        msg = str;
    }
    output_refresh(w, str);
}

void Windows::recv_msg(int w, stringstream &ss){
    pthread_mutex_lock( &mutex ); // 上鎖 
    msg_filter(w, ss.str());
    ss.str("");
    pthread_mutex_unlock( &mutex ); // 解鎖
}

Windows::~Windows(){
    nocbreak();
    echo();
    nl();
    refresh();
    endwin();
    pthread_mutex_destroy(&mutex);
}


#endif