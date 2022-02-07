#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
using namespace std;

int main(int argc, char *argv[])
{
	long client, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int l_clock = 0;
	char buffer[256];
	
	if(argc < 2) {
		fprintf(stderr,"usage %s port\n", argv[0]);
		exit(0);
	}
	
	portno = atoi(argv[1]);

	srand(time(0));						// Initiating the random function with current time as input
	l_clock = (rand()%25) + 5;				// Defining the range of random numbers from 5 to 30

	client = socket(AF_INET, SOCK_STREAM, 0);	//creating a socket
	
	if(client < 0) 
	{
		cout << "Error while opening the socket";
		return -1;
	}
	server = gethostbyname("localhost");
	
	if(server == NULL) {
		cout << "Given Host doesn't exist\n";
		exit(0);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if(connect(client,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		cout << "Error while connecting to server!";

	cout << "My logical Clock: " << l_clock << endl; // Printing machine's local logical clock

	bzero(buffer,256);
	n = read(client,buffer,255);	// Reading Time Daemon's Logical Clock
	if (n < 0) 
	{
		cout << "Error while reading from the socket";
		return -2;
	}
	
	cout << "Time Daemon Initiating Berkeley's Algorithm!\nThis is TD's logical clock: " << buffer << endl;	// Printing Time Daemon's Logical Clock
	
	stringstream ss, ss1;
	
	ss << buffer;
	string tmpstr1 = ss.str();
	
	int tmp = atoi(tmpstr1.c_str());	// converting Time Daemon's clock from char array to integer value
	
	int diff = l_clock - tmp;		// Calculating time difference of local machine from Time Daemon
	cout << "My Time Difference after comparing with Time Deamon's clock: "<< diff << endl;
	
	bzero(buffer,256);

	ss1 << diff;
	string tmpstr2 = ss1.str();
	strcpy(buffer,tmpstr2.c_str());

	n = write(client,buffer,strlen(buffer));	// Sending this machine's Time Difference to Time Daemon
	if (n < 0) 
	{
		cout << "Error while writing to the socket";
		return -1;
	}

	bzero(buffer,256);
	n = read(client,buffer,255);			// Reading the final average value to be adjusted in local machine's logical clock
	cout << "Required Clock Adjustment= "<<buffer << endl;

	int adj_clock = atoi(buffer);
	
	l_clock = l_clock + adj_clock;

	cout << "My Clock after being adjusted:" << l_clock << endl;

	close(client);	//Close the client socket and terminate the connection
	return 0;
}

