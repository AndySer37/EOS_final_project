CC = g++
TARGETS = socket_server

all: $(TARGETS)

socket_server:
	$(CC) -std=c++11 -o $@ $@.cpp -lpthread -lrt

clean:
# 	rm *.o
	rm $(TARGETS)
