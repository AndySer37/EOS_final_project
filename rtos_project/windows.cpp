// #include "project.h"
#include "sockop.h"
// #include "role.h"
#include "windows.h"
#include <unistd.h>
#include <cstdlib>
#include <cstring>


// int connfd;
// int debug(string str){
//     // cout<<"str: "<<str<<"--\n";
//     if(connfd == -1){
//         return -1;
//     }
//     else{
//         str+="\n";
//         if(write(connfd, str.c_str(), str.size()) == -1){
//             perror("Error: write()\n");
//             return -1;
//         }
//         return 0;
//     }
// }

int main(int argc, char** argv) {

    // if(argc == 3){
    //     connfd = connectsock(argv[1], argv[2], "tcp");
    // }
    // else if(argc == 2){
    //     connfd = connectsock("127.0.0.1", argv[1], "tcp");
    // }
    // else {
    //     connfd = -1;
    // }
    
    initial();
    Windows w;
    bool sth = 1;
    int i=0;
    w.chatroom();
    while(sth){
        w.recv_msg(w.input(sth));
    }
    
    endwin();

    // if(connfd != -1)
    //     close(connfd);

    return 0;
}
