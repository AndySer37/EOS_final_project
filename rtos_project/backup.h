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

using namespace std;
#define BUFSIZE 1024
#define SERSIZE 30
#define Server_output false

struct seat_inform {
	string kind;
	int num;
};

class console{
public:
    console(){}
    string name;
    vector<struct seat_inform> seat;
    void display();
};
void console::display(){
	if (Server_output){
		cout << name << endl;
	    for(vector<struct seat_inform>::iterator iter = seat.begin(); iter != seat.end(); iter++){
	        cout << iter->kind << " " << iter->num << endl;
	    }
	    cout << endl;
	}
}

class concert{
public:
    concert(){}
    string name;
    string cons;
    vector<struct seat_inform> seat;
    vector<struct seat_inform> left_seat;
    string display();
};
string concert::display(){
    stringstream ss;
    if (Server_output)
    	cout << name << endl;
    ss << name;
    for(vector<struct seat_inform>::iterator iter = seat.begin(); iter != seat.end(); iter++){
    	if (Server_output)
        	cout << iter->kind << " : " << iter->num << endl;
        ss << " " << iter->kind << " " << iter->num;
    }
    ss << endl;
    return ss.str();
}

class ticket{
public:
    ticket(){}
    ticket(string concert, string console, string region, int num, time_t time, time_t command_t){
        this->concert = concert;
        this->console = console;
        this->region = region;
        this->command_t = command_t;
        number = num;
        t = time;
    }
    string display();
    string concert;
    string console;
    string region;
    int number;
    time_t t;
    time_t command_t;
};
string ticket::display(){
    stringstream ss;
    time_t diss;
    diss = t - command_t;
	    ss << concert << " " << region << " " << region << number << " "
	        << setfill('0') << setw(1)
	        << (int)diss << endl;
    return ss.str();
}
class sys{
public:
    sys(){}
    sys(string, string, int);
    void order(string ,string ,int , time_t);
    void input(int);
    void output(int);
    void menu();
    void menu_out(int);
    vector<struct seat_inform>::iterator find_concert_iter(string, vector<struct seat_inform> *);
    int console_num;
    int concert_num;
    console *console_list;
    concert *concert_list;
    vector<ticket> tickets;
    time_t time1, time2;
    int sockfd;
    struct sockaddr_in addr_cln;
    socklen_t sLen;
    char snd[BUFSIZE], rcv[BUFSIZE];
    int n;

    ostringstream ss;
};

sys::sys(string console_path, string concert_path, int sock){
    sLen = sizeof(addr_cln);
    sockfd = sock;
    ss.str();
    fstream file;
    file.open (console_path.c_str(), ifstream::in);
    int i = 0;
    if (file.is_open()){
        file >> console_num;
        console_list = new console[console_num];
        while(i < console_num){
            file >> console_list[i].name;
            while(file.get() != '\n'){
                if(file.eof())
                    break;
                string s;
                int x;
                file >> s >> x;
                seat_inform inform;
                inform.kind = s;
                inform.num = x;
                console_list[i].seat.push_back(inform);
            }
            i++;
        }
        file.close();
    }
    else{
        cout<<"cannot open console.txt\n";
    }
    file.open (concert_path.c_str(),ifstream::in);
    if (file.is_open()){
        i = 0;
        file >> concert_num;
        concert_list = new concert[concert_num];
        while(i < concert_num){
            file >> concert_list[i].name;
            file >> concert_list[i].cons;
            while(file.get() != '\n'){
                if(file.eof())
                    break;
                string s;
                int x;
                file >> s >> x;
                seat_inform inform1, inform2;
                inform1.kind = s;
                inform1.num = x;
                inform2.kind = s;
                inform2.num = x;
                concert_list[i].seat.push_back(inform1);
                concert_list[i].left_seat.push_back(inform2);
            }
            i++;
        }
        file.close();
    }
    else{
        cout<<"cannot open concert.txt\n";
    }
}

void sys::input(int connfd){
    string concert, region, num, str;
    char c, dest[100];
    int number, kind = 0;
    concert = region = num = "";
    char *pch, *pch1;

    while(1){
    	ss.str("");
    	memset(rcv, 0, BUFSIZE);
    	memset(snd, 0, BUFSIZE);

        if((n = read(connfd, rcv, BUFSIZE)) == -1)
            errexit("Error: read()\n");
        time1 = std::time(0);

        n = sprintf(snd, "%.*s", n, rcv);
        cout << "Received from cilent " << connfd << " : " << snd << endl;
        str = snd;

        if (str == "exit"){
        	cout << "Client " << connfd << " exit !!" << endl;
        	break;
        }
        if (str == "showall"){
        	cout << "Client " << connfd << ": Showall !!" << endl;
        	menu_out(connfd);
        	continue;
        }
        number = strlen(snd);
        pch = strstr (snd,"show ");
        

        if (pch != NULL){
        	pch1 = strchr (snd, ' ')+1;
        	strncpy(dest, pch1, 25);//  number - strlen(pch1));
        	for(int i = 0; i < concert_num; i++){
        		pch = strstr (snd, concert_list[i].name.c_str());
        		if (pch != NULL){
        			ss << concert_list[i].display();
        		}
        	}
    		if (ss.str() == ""){
    			ss << dest << " not found" << endl;
    		}
        	if((n = write(connfd, ss.str().c_str(), BUFSIZE)) == -1)
        		errexit("Error: write()\n");
        	continue;
        }

        snd[number] = '\n';
        stringstream in_ss;
        cout << "==========" << snd << endl;
        in_ss << snd;

        in_ss.get(c);
        kind = 0;
     	number = 0;
        while(1){  	
            if(c == '/'){
            	cout << "/" << endl;
                kind++;    
                in_ss.get(c);
                continue;
            }
            else if(c == ' '){
            	cout << " " << endl;
                number = atoi(num.c_str());
                cout << concert << region << number << endl;
                order(concert, region, number, time1);
                concert = region = num = "";
                kind = 0;
                number = 0;
                in_ss.get(c);
                continue;
            }
            else if(c == '\n'){
            	cout << "end" << endl;
                number = atoi(num.c_str());
                order(concert, region, number, time1);
                output(connfd);
                concert = region = num = "";
                kind = 0;
                number = 0;
                in_ss.get(c);
                break;
            }
            if(kind == 0)
                concert += c;
            else if(kind == 1)
                region += c;
            else if(kind == 2)
                num += c;
            in_ss.get(c);
        }
    }
}

void sys::output(int connfd){

    for(int i = 0; i < tickets.size(); i++){
        ss << tickets[i].display();
    }
    for(int i = 0; i < concert_num; i++){
        concert_list[i].display();
    }
    if (Server_output){
	    cout << "==============\n";
	}

    if (strstr (ss.str().c_str(),"sorry, remaining ticket number not enough") != NULL){
        ss.str("");
        ss << "sorry, remaining ticket number not enough\n";
    }

    if((n = write(connfd, ss.str().c_str(), BUFSIZE)) == -1)
        errexit("Error: write()\n");
    cout << ss.str() << endl;
    ss.str("");

    tickets.clear();

    time_t time;
    time2 = std::time(0); 
    time = time2 - time1;
}
void sys::menu_out(int connfd){
    stringstream out;
    for(int i=0;i<concert_num;i++){
        out << concert_list[i].display();
    }
    //out << "===============\n";
    if((n = write(connfd, out.str().c_str(), BUFSIZE)) == -1)
        errexit("Error: write()\n");
}

vector<struct seat_inform>::iterator sys::find_concert_iter(string kind, vector<struct seat_inform> *seat){
	vector<struct seat_inform>::iterator iter;
	for (iter = seat->begin(); iter != seat->end(); ++iter){
		if (iter->kind == kind)
			break;
	}
	return iter;
}

void sys::order(string concert, string region, int number, time_t command_t){
    vector<struct seat_inform>::iterator iter, origin_iter;
    for(int i = 0; i < concert_num; i++){
        if(concert == concert_list[i].name){
            iter = find_concert_iter(region, &concert_list[i].seat);
            origin_iter = find_concert_iter(region, &concert_list[i].left_seat);
            if (iter != concert_list[i].seat.end()){
                if(iter->num >= number){
                    for(int j = 0; j < number; j++){
                        time_t t;
                        t = std::time(0); 
                        ticket ti(concert_list[i].name, concert_list[i].cons, region, origin_iter->num - iter->num, t, command_t);
                        tickets.push_back(ti);
                        iter->num--;
                    }
                    return ;
                }
                else{
                    ss << "sorry, remaining ticket number not enough\n";
                	return;
                }
            }
            else {
                ss << concert << " does not have " <<region << " ticket\n";
            	return;
            }
        }
    }
    // ss << concert << " not found\n";
    ss << "input format not valid\n";
}
