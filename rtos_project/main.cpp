#include "client.h"

int main(int argc, char *argv[]){
	if(argc != 3)
		errexit("Usage: %s concert console port\n", argv[0]);
	client client(argv[1], argv[2]);
	
}