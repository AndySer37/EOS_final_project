// #include "project.h"
#include "sockop.h"
// #include "role.h"
#include "windows.h"
#include <unistd.h>
#include <cstdlib>
#include <cstring>


int connfd;
int debug(string str){
    // cout<<"str: "<<str<<"--\n";
    if(connfd == -1){
        return -1;
    }
    else{
        str+="\n";
        if(write(connfd, str.c_str(), str.size()) == -1){
            perror("Error: write()\n");
            return -1;
        }
        return 0;
    }
}

int main(int argc, char** argv) {

    if(argc == 3){
        connfd = connectsock(argv[1], argv[2], "tcp");
    }
    else if(argc == 2){
        connfd = connectsock("127.0.0.1", argv[1], "tcp");
    }
    else {
        connfd = -1;
    }
    
    initial();
    // while(1){
    //     if (kbhit()) {
    //         char x=getch();
    //         if(x == 27)
    //             break;
    //         else if(x == 127)
    //             cout<<"backapsce\n";
    //         else if(x == '\r')
    //             cout<<"enter\n";
    //         else
    //             cout<<x<<endl;
    //     }
    // }





    Windows w;

    w.chatroom();
    
    endwin();

    if(connfd != -1)
        close(connfd);

    return 0;
}


void initial()
{
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
    nonl();
    curs_set(0);
    nodelay(stdscr,TRUE);
    noecho();
    // echo();
    intrflush(stdscr,FALSE);
    keypad(stdscr,TRUE);
    refresh();
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