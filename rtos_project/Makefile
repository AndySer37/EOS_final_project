CC = g++
TARGETS = socket_client windows game_server/gameserver

all: $(TARGETS)

# $(OBJS).o: $(OBJS).c
# 	$(CC) -c -o $@ $<
game_server/gameserver: game_server/socket_server.cpp
	$(CC) -std=c++11 -o $@ $^ -lpthread -lrt

socket_client:
	$(CC) -std=c++11 -o $@ $@.cpp  -lpthread -lrt -lncurses

windows:
	$(CC) -std=c++11 -o $@ $@.cpp  -lpthread -lrt -lncurses

clean:
# 	rm *.o
	rm $(TARGETS)
