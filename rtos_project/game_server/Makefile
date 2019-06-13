CC = g++
OBJS = socket_server.cpp
TARGETS = game_server

all: $(TARGETS)

game_server: $(OBJS)
	$(CC) -std=c++11 -o $@ $^ -lpthread -lrt

clean:
# 	rm *.o
	rm $(TARGETS)
