#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

using namespace std;

char buffer[256];
int n;

int l_clock = 0, tot = 0, avg = 0, cnt = 0, t=0;

void *Connections(void *newserver);
void BerkeleyAlgo(long newserver);

int main(int argc, char *argv[])
{
	long server, newserver[10], portno;
	socklen_t clilen;
	
	struct sockaddr_in serv_addr, cli_addr;

	pthread_t threads[10];	//threads for handling each process separately

	srand(time(0));						// Initiating the random function with current time as input
	l_clock = (rand()%25) + 5;				// Defining the range of random numbers from 5 to 30

	if (argc < 2) 
	{
		cout << "Port number is not provided\n";
		return -1;
	}

	server = socket(AF_INET, SOCK_STREAM, 0);	//creating a socket
	if(server < 0) 
	{
		cout << "Error while opening the socket";
		return -1;
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	if(bind(server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)	//binding a socket
	{	
		cout <<"Error while binding the socket";
		return -1;
	}
	
	if(listen(server, 10) < 0)
	{
		cout << "Cannot listen"<<endl;
		return -3;
	}
	
	clilen = sizeof(cli_addr);

	cout << "Enter the total number of machines to connect: ";
	cin >> t;

	cout << "Time Daemon's Logical Clock: " << l_clock << endl;

	int sockid = 0;

	for(int i=0; i < t; i++)
	{
		newserver[sockid] = accept(server,(struct sockaddr *) &cli_addr,&clilen);

		pthread_create(&threads[i], NULL, Connections, (void *)newserver[sockid]);
		
		sockid=(sockid+1)%100;
	}
	
	for(int i=0; i<t; i++)
	{
		int rc = pthread_join(threads[i], NULL);
		if(rc)
		{
			cout << "Error while joining the thread :\n";
			cout << "Error: " << rc << endl;
			exit(-1);
		}
	
	}

	cout << "Required Clock Adjustment: " << avg << endl;
	l_clock = l_clock + avg;

	cout << "My Clock after being adjusted: " << l_clock << endl;

	close(server);
	pthread_exit(NULL);
	return 0; 
}





void BerkeleyAlgo(long newserver)
{
	bzero(buffer,256);

	stringstream ss, ss1, ss2;
	ss << l_clock;
	string tmpstr1 = ss.str();				// Converting clock value from int to string or char array
	strcpy(buffer,tmpstr1.c_str());				// Now converting from string to const char *
	
	n = write((long)newserver,buffer,strlen(buffer));	// Sending Time Daemon's logical clock to connected machine
	if (n < 0)
	{
		cout << "Error while writing to the socket";
		exit(0);
	}
	bzero(buffer,256);
	read((long)newserver,buffer,255);			//Reading the specific time difference of connected machines
	cout << "Time Difference of the Machine '" << newserver << "' : " << buffer << endl;
	
	ss1 << buffer;
	string tmpstr2 = ss1.str();
	
	int diff = atoi(tmpstr2.c_str());	//converting Time Daemon's clock from char array to integer value

	tot = tot + diff;			//Adding all time differences

	sleep(2);

	avg = tot/(cnt+1);			//Taking average of the total time differences

	int adj_clock = avg - diff;		//Calculating the average time adjustment for each clock

	bzero(buffer,256);
	
	ss2 << adj_clock;
	string tmpstr3 = ss2.str();
	strcpy(buffer,tmpstr3.c_str());		//Converting time adjustment value from integer to const char *
	n = write((long)newserver,buffer,strlen(buffer));	//Sending specific time adjustment to corresponding machine
	if (n < 0) 
	{
		cout << "Error while writing to the socket";
		exit(0);
	}
}


void *Connections(void *newserver) //thread function for each client request
{
	if((long)newserver < 0)
	{
		cout << "Error while accepting the connection";
		exit(0);
	}
		

	
	cout << "Connected to the Machine Number: " << (long)newserver << endl;
	cnt++;

	while(cnt != t)
	{
		continue;
	}

	BerkeleyAlgo((long)newserver);

	close((long)newserver);
	pthread_exit(NULL);
}


