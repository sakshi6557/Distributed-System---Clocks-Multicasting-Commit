#compiler
CC = g++

compile : server.o client.o server1.o client1.o causal.o noncausal.o

server.o: berkserver.cpp
	g++ -o server berkserver.cpp -lpthread

client.o: berkclient.cpp
	g++ -o client berkclient.cpp -lpthread
	
server1.o: server_bonus.cpp
	g++ -o server1 -g server_bonus.cpp -lpthread

client1.o: client_bonus.cpp
	g++ -o client1 -g client_bonus.cpp -lpthread
	
causal.o:
	g++ -o causal Multicast_Causal.cpp -lpthread

noncausal.o:
	g++ -o noncausal Multicast_NonCausal.cpp -lpthread

clean:
	rm -rf server client server1 client1 causal noncausal
