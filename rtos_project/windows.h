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
const string role_name[] = {"???", "Police", "Detective", "Bodyguard", "Doctor", "Spy", "Retired soldier"\
                    , "Godfather", "Intimidate", "Streetwalker", "Survivor", "Serial killer"};
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
    "You have totally twice chance to be on the alert at night, and you will kill all the person who visit you instead of serial killer.",
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
        is_alive = false;
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
    void player_info_refresh(int ,int, int, bool, string );
    void output_refresh(int ,string );
    void playerlist_refresh();
    void lobby_refresh(int , string );
    void name_setted();
    void msg_filter(string );
    void recv_msg(stringstream & );
    bool is_player_alive(int i){ return p[i].is_alive; }
    void set_statr_ptr(int &ptr){ state = &ptr; }
    void set_role_tab_info(int, stringstream & );
    string input();
    string char_intput(int );

private:
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    WINDOW *chat_o, *chat_i, *plist, *sysmsg, *lobby;
    WINDOW *chat_o_box, *chat_i_box, *sysmsg_box, *plist_box;
    
    bool is_plist_show = false;
    bool is_game_start = false;
    bool is_name_set = false;

    string msg;
    Player_info p[11];
    int *state;
    int self_id;
    string role_tab_info[5];
};

Windows::Windows(){
    pthread_mutex_init (&mutex, NULL);

    initial();
    plist=newwin(32,76,(LINES)/2-20,(COLS-76)/2);
    plist_box=newwin(34,82,(LINES)/2-21,(COLS-82)/2);
    nodelay(plist,TRUE);
    box(plist_box,'O','O');
    touchwin(plist_box);
    wrefresh(plist_box);
    touchwin(plist);
    wrefresh(plist);
    playerlist_refresh();

    sysmsg=newwin(15,78,(LINES)/2-20,(COLS-78)/2);
    sysmsg_box=newwin(17,82,(LINES)/2-21,(COLS-82)/2);
    nodelay(sysmsg,TRUE);
    scrollok(sysmsg,TRUE); 
    box(sysmsg_box,'*','*');
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
void Windows::player_info_refresh(int flag, int i, int role, bool alive, string n){
    if(flag == 0){
        p[i].role_id = role;
    }
    else if(flag == 1){
        p[i].is_alive = alive;
    }
    else if(flag ==2){
        p[i].id = i;
        p[i].name = n;
        p[i].is_playing = true;
    }
}

void Windows::set_role_tab_info(int i, stringstream & ss){
    role_tab_info[i] = ss.str();
    ss.str("");
    wattron(plist,COLOR_PAIR(7));
    mvwprintw(plist,3+i, 0, "%s", role_tab_info[i].c_str());
    mvwprintw(plist,14,12,"|  I D  |           name           |      role      |");
    touchwin(plist);
    wrefresh(plist);
}

void Windows::playerlist_refresh(){
    if(is_plist_show){
        wattron(plist,COLOR_PAIR(7));
        for(int i=0,j=0;j<11;j++){
            if(p[j].is_playing){
                if(p[j].is_alive){
                    wattron(plist, A_BOLD);
                }
                else{
                    wattron(plist, A_DIM);
                }
                mvwprintw(plist,15+i, 15,"%d",p[j].id);
                mvwprintw(plist,15+i, 22,"%s", p[j].name.c_str());
                mvwprintw(plist,15+i, 49,"%s", role_name[p[j].role_id].c_str());
                if(p[j].is_alive){
                    wattroff(plist, A_BOLD);
                }
                else{
                    wattroff(plist, A_DIM);
                }
                i++;
            }
        
        }
        touchwin(plist_box);
        wrefresh(plist_box);
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
        touchwin(chat_o);
        wrefresh(chat_o);
    }
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
}

void Windows::lobby_refresh(int i, string n){
    if(!is_game_start){
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
            string tag( str.substr(0,str.find_first_of(')', 0)+1) );
            if(tag == "(3)"){
                wattron(sysmsg,COLOR_PAIR(3)|A_BOLD);
                str.erase (0,tag.size());
            }
            else if(tag == "(2)"){
                wattron(sysmsg,COLOR_PAIR(2));
                str.erase (0,tag.size());
            }
            else if(tag == "(1)"){
                wattron(sysmsg,COLOR_PAIR(3));
                str.erase (0,tag.size());
            }
            else{
                wattron(sysmsg,COLOR_PAIR(7));
            }
            mvwprintw(sysmsg, y, 0, str.c_str());
            if(tag == "(3)"){
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
    int x, last_state;
    msg.clear();
    if(state != NULL){
        last_state = *state;
    }
    while(1){
        if((state != NULL)&&(last_state != *state)){
            msg.clear();
            char_intput('\r');
            return "";
        }
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
                if(is_game_start){
                    is_plist_show = !is_plist_show;
                    playerlist_refresh();
                }
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
            mvwprintw(chat_i,2,0,"Enter your name :");
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

void Windows::name_setted(){
    is_name_set = true;
    is_game_start = true;
}
void Windows::msg_filter(string str){
    int w;
    string tag( str.substr(0,str.find_first_of(')', 0)+1) );
    if(tag == string("(s)")){
        str.erase (0,tag.size());
        w = 1;
    }
    else if(tag == string("(p)")){
        str.erase (0,tag.size());
        w = 0;
    }
    else{
        w = 1;
    }
    output_refresh(w, str);
}

void Windows::recv_msg(stringstream &ss){
    pthread_mutex_lock( &mutex ); // 上鎖 
    msg_filter(ss.str());
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