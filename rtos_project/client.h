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

client::client(char *ip, char *port){
    connfd = connectsock(ip, port, "tcp");
    if(connfd == -1)
        errexit("Error: accept()\n"); 
    system("clear");
    cout << "Your connected connfd is : " << connfd << endl;
    Role r(connfd, 1, 0);
    r.save_ptr(&r);
}