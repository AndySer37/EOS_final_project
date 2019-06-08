#include "game_server.h"

using namespace std;

int main(int argc, char *argv[]){
	// if(argc != 3)
	// 	errexit("Usage: %s concert console port\n", argv[0]);

	int** tb = new int*[ROLE_AMO];
	for (int i = 0;i < ROLE_AMO; i++){
		tb[i] = new int [3];
		tb[i][0] = i;
		tb[i][1] = DEFAULT;
		tb[i][2] = 1;
	}
	game_server game_server(11, tb);

	////// checking vote ////////
	string str_arr[game_server.alive] = {"--p 0 --vote 2", "--p 1 --vote -1", "--p 2 --vote 1"\
	, "--p 6 --vote 7", "--p 9 --vote 1", "--p 10 --vote -1", "--p 3 --vote 6"
	, "--p 4 --vote 7", "--p 7 --vote 4", "--p 8 --vote 7", "--p 5 --vote 9"};
	game_server.vote_update(str_arr);
	cout << "\n\nEvent_des : \n\n";
	cout << game_server.event_des << endl;
	cout << "\n------------------------------\n";

	////// checking night_update ////////
	game_server.alive ++;
	string str_arr1[game_server.alive] = {"--role 0 --obj 10", "--role 1 --obj 7", "--role 2 --obj 1"\
	, "--role 6 --obj 5", "--role 9 --obj -1", "--role 10 --obj 6", "--role 3 --obj 5"\
	, "--role 4 --obj 5", "--role 7 --obj 5", "--role 8 --obj 10", "--role 5 --obj -2"};
	game_server.night_update(str_arr1);   
	for (int i = 0; i < ROLE_AMO ; i++){
		cout << i << ": " << game_server.respond[i];
	}
	if (game_server.intimidate_obj >= 0){
		cout << "Player " << game_server.intimidate_obj << " will be prohibited to chat\n";
	} 
	cout << "\n\nEvent_des : \n\n";
	cout << game_server.event_des << endl;

	//// check god father alive ////
	bool check = game_server.godfather_alive_check();
	if (check){
		cout << "god father dead.\n";
		cout << "next god father player : " << game_server.role_table[6][0] << endl;
		cout << "command :--gf\n";
	}
	//// check game over ////
	check = game_server.game_over_check();

}