#ifndef _WINDOWS_H_
#define _WINDOWS_H_
#include <iostream>
#include <ncurses.h>
#include <string>
#include <time.h>
using namespace std;

void initial();
int kbhit ();

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
    Chatroom(Player *p);
    void output_reflesh();
    void output_reflesh(string msg);
    void output_reflesh(string name, string msg);
    void send_msg(string msg);
    void recv_msg(string msg);
    void intput(int c);
    WINDOW *chat_o, *chat_i;
    WINDOW *chat_o_box, *chat_i_box;
    string msg;
    Player *p_self;
private:
};

Chatroom::Chatroom(){
    chat_o=newwin(30,40,(LINES)/2-20,(COLS-40)/2+20);
    chat_o_box=newwin(32,42,(LINES)/2-21,(COLS-40)/2+19);
    box(chat_o_box,'*','*');
    // mvwprintw(chat_o,2,2,"output\n");
    scrollok(chat_o,TRUE); 
    touchwin(chat_o_box);
    wrefresh(chat_o_box);
    touchwin(chat_o);
    wrefresh(chat_o);

    chat_i=newwin(5,40,(LINES)/2+12,(COLS-40)/2+20);
    chat_i_box=newwin(7,42,(LINES)/2+11,(COLS-40)/2+19);
    box(chat_i_box,'|','-');
    mvwprintw(chat_i,2,2,"Say :");
    scrollok(chat_i,TRUE);
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
};

Chatroom::Chatroom(Player *p){
    p_self = p;
    chat_o=newwin(30,40,(LINES)/2-20,(COLS-40)/2+20);
    chat_o_box=newwin(32,42,(LINES)/2-21,(COLS-40)/2+19);
    box(chat_o_box,'*','*');
    // mvwprintw(chat_o,2,2,"output\n");
    scrollok(chat_o,TRUE); 
    touchwin(chat_o_box);
    wrefresh(chat_o_box);
    touchwin(chat_o);
    wrefresh(chat_o);

    chat_i=newwin(5,40,(LINES)/2+12,(COLS-40)/2+20);
    chat_i_box=newwin(7,42,(LINES)/2+11,(COLS-40)/2+19);
    box(chat_i_box,'|','-');
    mvwprintw(chat_i,2,2,"Say :");
    scrollok(chat_i,TRUE);
    touchwin(chat_i_box);
    wrefresh(chat_i_box);
    touchwin(chat_i);
    wrefresh(chat_i);
};

void Chatroom::intput(int c){
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
        mvwprintw(chat_i,2,2,":");
        msg.clear();
    }
    else{
        wprintw(chat_i, "%c", c);
        msg += c;
    }
    touchwin(chat_i);
    wrefresh(chat_i);
}
void Chatroom::output_reflesh(){
    touchwin(chat_o);
    wrefresh(chat_o);
}
void Chatroom::output_reflesh(string str){
    int x, y;
    touchwin(chat_o);
    getyx(chat_o, y, x);
    str +="\n";
    mvwprintw(chat_o, y, 2, str.c_str());
    touchwin(chat_o);
    wrefresh(chat_o);
}
void Chatroom::output_reflesh(string name, string str){
    int x, y;
    touchwin(chat_o);
    getyx(chat_o, y, x);
    str +="\n";
    mvwprintw(chat_o, y, 2, name.c_str());
    mvwprintw(chat_o, y, 10, str.c_str());
    touchwin(chat_o);
    wrefresh(chat_o);
}


void Chatroom::send_msg(string msg){
    output_reflesh(p_self->name, msg);
    //TODO
}
void Chatroom::recv_msg(string msg){
    output_reflesh(msg);
}










class Windows
{
public:
    Windows();
    void playerlist();
    void chatroom();
private:
    WINDOW *plist;
    Chatroom *chat_room;
    Player p[11], *p_self;
};

Windows::Windows(){
    plist=newwin(39,40,(LINES)/2-21,(COLS-40)/2-25);
    nodelay(plist,TRUE);
    box(plist,'|','-');
    p_self = &p[0];
}

void Windows::playerlist(){
    mvwprintw(plist,6,9,"|   I D   |   role   | live |");
    for(int i=0,j=0;j<11;j++){
        if(p[j].alive){
            wattron(plist,COLOR_PAIR(7)|A_BOLD);
        }
        else{
            wattron(plist,COLOR_PAIR(6));
        }
        // char *name=new char [p[j].name.size()+1];
        // strcpy (name,p[j].name.c_str());
        mvwprintw(plist,7+i, 5," %d ",i+1);
        mvwprintw(plist,7+i, 10," %-8s ", p[j].name.c_str());
        // wattroff(plist,COLOR_PAIR(6)|A_BOLD);
        if(p[j].show_role){
            // wattron(plist,COLOR_PAIR(3)|A_BOLD);
            mvwprintw(plist,7+i,26,"  ?  ");
        }
        else{
            // wattron(plist,COLOR_PAIR(4)|A_BOLD);
            mvwprintw(plist,7+i,26,"%d", p[j].Role_id);
        }
        i++;
        
    }
    // wattroff(plist,COLOR_PAIR(3)|COLOR_PAIR(4));
    // wattroff(plist,A_BOLD);
    touchwin(plist);
    wrefresh(plist);
}

void Windows::chatroom(){
    chat_room = new Chatroom(p_self);
    playerlist();
    int x;
    while(1){
        chat_room->output_reflesh();
        if (kbhit()) {
            x=getch();
            if(x == 27){
                break;
            }
            else{
                chat_room->intput(x);
            }
        }
    }
}

#endif