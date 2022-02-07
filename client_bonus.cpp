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

#define REQUEST 1
#define RELEASE 2
#define FINISH 3

using namespace std;

int main(int argc, char *argv[])
{

	long client, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int req;
	char buffer[256];
	
	if(argc < 2) {
		fprintf(stderr,"usage %s port\n", argv[0]);
		exit(0);
	}
	
	portno = atoi(argv[1]);

	client = socket(AF_INET, SOCK_STREAM, 0);	//creating a socket
	if(client < 0) 
	{
		cout << "Error while opening the socket";
		return -4;
	}
	
	server = gethostbyname("localhost");
	if(server == NULL) {
		cout << "Given Host doesn't exist\n";
		return -1;
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if(connect(client,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		{
		cout << "Error while connecting to server!";
		return -2;
	}
	int a = 0;

	while(a < 5)
	{
		req = REQUEST;
		n = write(client,&req,sizeof(int));				// Sending Request to Access Shared File
		if (n < 0) 
		{
			cout << "Error while writing to the socket";
			exit(0);
		}
		
		cout << "Trying to Access Shared File!" << endl; 
		
		n = read(client,&req,sizeof(int));				// Reading Centralized Server's Response
		if (n < 0) 
		{
			cout << "Error while reading from the socket";
			exit(0);
		}
		cout << "Got access to the file!" << endl;			// Printing Server's Response
	
		ifstream shared_file("CriticalSection.txt");
	
		string line;
		int cnt;

		if(shared_file.is_open())
		{
			getline(shared_file, line);				//Reading file by line and storing the values
			istringstream ss(line);
			ss >> cnt;
			shared_file.close();
		}
		else cout << "Unable to open the file"; 
	
		sleep(2);
	
		cout << "Initial Counter Value: " << cnt << endl;
		cnt++;

		ofstream my_shared_file("CriticalSection.txt");
		if(my_shared_file.is_open())
		{
			my_shared_file << cnt;				// Updating file with incremented counter value
			my_shared_file.close();
		}
		else
			cout << "Unable to open file";
	
		ifstream shared_file1("CriticalSection.txt");
	
		if(shared_file1.is_open())
		{
			getline(shared_file1, line);			//Reading file by line and storing the values
			istringstream ss1(line);
			ss1 >> cnt;
			shared_file1.close();
		}
		else cout << "Unable to open file"; 
		cout << "Updated Counter Value: " << cnt << endl;
		req = RELEASE;
		n = write(client,&req,sizeof(int));			// Sending Message to Release Shared File
		if (n < 0) 
		{
			cout << "Error while writing to the socket";
			exit(0);
		}

		cout << "Released the File Access!\n" << endl;

		a++;
	}
	req = FINISH;
	n = write(client,&req,sizeof(int));				// Sending Request to Access Shared File
	if (n < 0) 
	{
		cout << "Error while writing to the socket";
		exit(0);
	}

	cout << "Work Finished. Closing Connection!" << endl;
	

	close(client);							//Close the client socket and terminate the connection
	return 0;
}

