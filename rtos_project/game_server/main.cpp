#include "game_server.h"

using namespace std;

int main(int argc, char *argv[]){
	// if(argc != 3)
	// 	errexit("Usage: %s concert console port\n", argv[0]);

	int** tb = new int*[ROLE_AMO];
	for (int i = 0;i < ROLE_AMO; i++){
		tb[i] = new int [2];
		tb[i][0] = i;
		tb[i][1] = NOT_USE;
	}
	game_server game_server(11, tb);
}