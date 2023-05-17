CPP=g++
CPPFLAGS=-std=c++11 -Wall


all: client server

client: client.cpp
	$(CPP) $(CPPFLAGS) -o client client.cpp 

server: server.cpp
	$(CPP) $(CPPFLAGS) -o server server.cpp 

clean:
	rm -f client server
