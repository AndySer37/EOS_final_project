#include "hw2.h"


void *connection_handler(void *);
struct  data{
	int connfd;
	int count;
	class sys *ss;
};


int main(int argc, char *argv[]){


    int sockfd, connfd, count = 0, count_client = 0;
	struct sockaddr_in addr_cln;
	socklen_t sLen = sizeof(addr_cln);
	int n;

	if(argc != 4)
		errexit("Usage: %s concert console port\n", argv[0]);

	sockfd = passivesock(argv[3], "tcp", 10);
	sys sss(argv[1], argv[2], sockfd);
	sys* ptr = &sss;
	cout << "Successfully load file \n";

	pthread_t thread_id[SERSIZE];
	struct data a[SERSIZE];

    while(1){
		connfd = accept(sockfd, (struct sockaddr *) &addr_cln, &sLen);
		if(connfd == -1)
			errexit("Error: accept()\n");
		cout << "Number of Client: " << count_client << endl;
		a[count].connfd = connfd;
		a[count].count = count_client;
		a[count].ss = ptr;
        if( pthread_create( &thread_id[count] , NULL ,  connection_handler , (void*) &a[count]) < 0){
            perror("could not create thread");
            return 1;
        }
        // /pthread_detach( thread_id[count] );
        count ++;
        count_client ++;
        if(count >= SERSIZE)
        	count = 0;
    }
    cout << "Server closed !!\n";
    return 0;
}


void *connection_handler(void *conn){

    struct data a = *(data*)conn;
	int n;
	char snd[BUFSIZE], rcv[BUFSIZE];
	sys* ptr =  a.ss;

	// n = sprintf(snd, "exit", n);
	cout << "Accept Client " << a.connfd << endl;

    a.ss->input(a.connfd);

	close(a.connfd);
	pthread_exit((void *)conn);
    return 0;
}

