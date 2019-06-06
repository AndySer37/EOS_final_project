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
#include <stdio.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/sem.h>

using namespace std;
#define BUFSIZE 1024
#define SERSIZE 30

int sockfd, connfd;
int n, count = 0;
char snd[BUFSIZE], rcv[BUFSIZE];

string input(){
    string kb_input = "";
    char c;
    cin.get(c);
    while(1){   
        if(c == '\n'){
            break;
        }
        kb_input += c;
        cin.get(c);
    } 
    return kb_input;   
}

void output(string out){
    if ((n = write(connfd, out.c_str(), strlen(out.c_str()))) == -1)
        errexit("Error: write()\n");    

}
int main(int argc, char *argv[]){


	int count_client = 0;
	struct sockaddr_in addr_cln;
	socklen_t sLen = sizeof(addr_cln);
	if(argc != 2)
		errexit("Usage: %s concert console port\n", argv[0]);

	sockfd = passivesock(argv[1], "tcp", 10);
	connfd = accept(sockfd, (struct sockaddr *) &addr_cln, &sLen);
    switch(fork()) {
        case -1:
            /* Fork Error */
            syslog(LOG_ERR, "Can't create child process");
            close(connfd);
            break;
        case 0:
            /* Child Process */
            while(1){
                string kb_input = input();
                output(kb_input);
            }
        default:
            //Parent Process 
			while(1){
				memset(rcv, 0, BUFSIZE);
				memset(snd, 0, BUFSIZE);
				if((n = read(connfd, rcv, BUFSIZE)) == -1)
					errexit("Error: read()\n");
				cout << rcv << endl;

			}
            break;
    }

}