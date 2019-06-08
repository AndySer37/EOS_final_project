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
#include "role.h"

using namespace std;
#define BUFSIZE 1024
#define SERSIZE 30
#define Server_output false

class client{
  public:
    client(){}
    client(char *, char *);
    ~client(){}
    int connfd;
    Role *my_role;

};

// build up game client
// example :--role 9 --p 16 --amo 11

client::client(char *ip, char *port){
    connfd = connectsock(ip, port, "tcp");
    if(connfd == -1)
        errexit("Error: accept()\n"); 
    system("clear");
    cout << "Your connected connfd is : " << connfd << endl;
    cout << "Waiting for game started !\n";

    char snd[BUFSIZE], rcv[BUFSIZE];
    char *pch, *pch1;
    string role, player;
    role = player = "";
    int n;
    memset(rcv, 0, BUFSIZE);
    memset(snd, 0, BUFSIZE);
     
    if((n = read(connfd, rcv, BUFSIZE)) == -1)
        errexit("Error: read()\n");
    pch = strstr (rcv,"--role ");
    pch1 = strstr (rcv,"--p ");
    for (int i = 0; i < (pch1 - pch - 8) ; i++){
        role += *(pch + 7 + i);
    }
    pch = strstr (rcv,"--amo ");

    for (int i = 0; i < (pch - pch1 - 5) ; i++){
        player += *(pch1 + 4 + i);
    }
    char *player_amount = (pch + 5);
    //cout << "role: " <<  atoi(role.c_str()) << ",player: " << atoi(player.c_str()) << ",player_amount: " << atoi(player_amount) << endl;
    Role r(connfd, atoi(role.c_str()), atoi(player.c_str()), atoi(player_amount));
    r.save_ptr(&r);
}

