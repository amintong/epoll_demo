CC = gcc
CXX = g++
CFLAGS = -std=c++11 -pthread -g
LDFLAGS = -lstdc++ 

all: server client

server: server.cpp
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS)

client: client.cpp
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f server client

