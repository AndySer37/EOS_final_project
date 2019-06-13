CC = g++
TARGETS = socket_client windows game_server/socket_server

all:
	$(CC) -std=c++11 -o game_server/socket_server game_server/socket_server.cpp -lpthread -lrt
	$(CC) -std=c++11 -o socket_client socket_client.cpp  -lpthread -lrt -lncurses
	$(CC) -std=c++11 -o windows windows.cpp  -lpthread -lrt -lncurses
	
game_server/socket_server:
	$(CC) -std=c++11 -o $@ $@.cpp -lpthread -lrt

socket_client:
	$(CC) -std=c++11 -o $@ $@.cpp  -lpthread -lrt -lncurses

windows:
	$(CC) -std=c++11 -o $@ $@.cpp  -lpthread -lrt -lncurses

clean:
# 	rm *.o
	rm $(TARGETS)
