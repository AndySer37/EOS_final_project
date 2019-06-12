#ifndef _WINDOWS_H_
#define _WINDOWS_H_
#include <iostream>
#include <sstream>
#include <ncurses.h>
#include <string>
#include <time.h>
using namespace std;

void initial()
{

}

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


class Player
{
public:
    Player(){
        name = "123";
    }
    int alive;
    int show_role;
    string name;
    int Role_id;
private:
};

class Chatroom
{
public:
    Chatroom();
    // void output_reflesh();
    void output_reflesh(int w,string msg);
    void playerlist_reflesh();
    void send_msg(string msgs);
    void recv_msg(int w, string msgs);
    string intput(int c);
    string msg;
    bool plist_show = false;
private:
    WINDOW *chat_o, *chat_i, *plist, *sysmsg;
    WINDOW *chat_o_box, *chat_i_box, *sysmsg_box;
    Player p[11], *p_self;
};

Chatroom::Chatroom(){

    wclear(stdscr);
    refresh();
    p_self = &p[0];
    plist=newwin(16,30,(LINES-16)/2,(COLS-30)/2);
    nodelay(plist,TRUE);
    box(plist,'|','-');
    playerlist_reflesh();

    sysmsg=newwin(15,80,(LINES)/2-20,(COLS-80)/2);
    sysmsg_box=newwin(17,82,(LINES)/2-21,(COLS-82)/2);
    nodelay(sysmsg,TRUE);
    box(sysmsg_box,'*','*');
    // mvwprintw(sysmsg,2,2,"output\n");
    scrollok(sysmsg,TRUE); 
    touchwin(sysmsg_box);
    wrefresh(sysmsg_box);
    touchwin(sysmsg);
    wrefresh(sysmsg);

    chat_o=newwin(15,80,(LINES)/2-3,(COLS-80)/2);
    chat_o_box=newwin(17,82,(LINES)/2-4,(COLS-82)/2);
    nodelay(chat_o,TRUE);
    box(chat_o_box,'|','-');
    // mvwprintw(chat_o,2,2,"output\n");
    scrollok(chat_o,TRUE); 
    touchwin(chat_o_box);
    wrefresh(chat_o_box);
    touchwin(chat_o);
    wrefresh(chat_o);

    chat_i=newwin(5,80,(LINES)/2+14,(COLS-80)/2);
    chat_i_box=newwin(7,82,(LINES)/2+13,(COLS-82)/2);

    noecho();
    nodelay(chat_i,TRUE);
    intrflush(chat_i,FALSE);
    keypad(chat_i,TRUE);
    nodelay(chat_i,TRUE);
    box(chat_i_box,'|','-');
    mvwprintw(chat_i,2,2,"Say :");
    scrollok(chat_i,TRUE);
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
};

void Chatroom::playerlist_reflesh(){
    if(plist_show){
        wattron(plist,COLOR_PAIR(7)|A_BOLD);
        mvwprintw(plist,2,4,"|  I D  |   role   |");
        for(int i=0,j=0;j<11;j++){
            if(p[j].alive){
                wattron(plist,COLOR_PAIR(7)|A_BOLD);
            }
            else{
                wattron(plist,COLOR_PAIR(6));
            }
            mvwprintw(plist,3+i, 2," %d ",i+1);
            mvwprintw(plist,3+i, 5," %-8s ", p[j].name.c_str());
            if(p[j].show_role){
                mvwprintw(plist,3+i,18,"  ?  ");
            }
            else{
                mvwprintw(plist,3+i,18,"%d", p[j].Role_id);
            }
            i++;
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
        touchwin(chat_o);
        wrefresh(chat_o);
    }
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
}

string Chatroom::intput(int c){
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
        send_msg(msg);  //send msg;
        wclear(chat_i);
        mvwprintw(chat_i,2,2,"Say :");
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
// void Chatroom::output_reflesh(){
//     playerlist_reflesh();
//     touchwin(chat_o);
//     wrefresh(chat_o);
//     touchwin(chat_i);
//     wrefresh(chat_i);
// }
void Chatroom::output_reflesh(int w, string str){
    if(str.empty()){
        // playerlist_reflesh();
        // touchwin(chat_o);
        // wrefresh(chat_o);
        // touchwin(chat_i);
        // wrefresh(chat_i);
    }
    else{
        if(w == 0){
            int x, y;
            touchwin(chat_o_box);
            wrefresh(chat_o_box);
            touchwin(chat_o);
            getyx(chat_o, y, x);
            str +="\n";
            mvwprintw(chat_o, y, 2, str.c_str());
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
            str +="\n";
            mvwprintw(sysmsg, y, 2, str.c_str());
            wrefresh(sysmsg);
            touchwin(chat_i_box);
            wrefresh(chat_i_box);
            touchwin(chat_i);
            wrefresh(chat_i);
        }
        playerlist_reflesh();
    }   
}

void Chatroom::send_msg(string msgs){

}
void Chatroom::recv_msg(int w, string msgs){
    output_reflesh(w, msgs);
}

class Windows
{
public:
    Windows();
    ~Windows();
    void chatroom();
    string input();
    string input(bool &);
    void recv_msg(int , string );
    void recv_msg(int , stringstream &);
private:
    Chatroom *chat_room;
};

Windows::Windows(){
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
    refresh();
}

Windows::~Windows(){
    nocbreak();
    echo();
    nl();
    refresh();
    endwin();
}

void Windows::chatroom(){
    chat_room = new Chatroom();
}

string Windows::input(){
    bool b = true;
    return input(b);
}
string Windows::input(bool &sth){
    int x;
    chat_room->msg.clear();
    while(1){
        if (kbhit()) {
            x=getch();
            if(x == 27){
                sth = 0;
                touchwin(stdscr);
                endwin();
                return "";
            }
            else if(x == '\r'){
                return chat_room->intput(x);
            }
            else if(x == '\t'){
                chat_room->plist_show = !(chat_room->plist_show);
                chat_room->playerlist_reflesh();
            }
            else{
                chat_room->intput(x);
            }
        }
        if(!sth){
            // touchwin(stdscr);
            // endwin();
            // break;
            return "";
        }
    }
}

void Windows::recv_msg(int w, string msg){
    chat_room->recv_msg(w, msg);
}
void Windows::recv_msg(int w, stringstream &ss){
    chat_room->recv_msg(w, ss.str());
    ss.str("");
}
#endif