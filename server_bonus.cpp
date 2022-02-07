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
#include <queue>
#include <list>

using namespace std;

#define REQUEST 1
#define RELEASE 2
#define FINISH 3
#define OK 4

queue<long> que;							// queue for maintaining the requests
list<long> llist;							// list for maintaining the active clients
int cs = 0;
pthread_mutex_t lock1, lock2;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void *Connections(void *newserver);
void *AccessProvider(void *arg);


int main(int argc, char *argv[])
{
	int t=0;
	long server, newserver[10], portno;
	socklen_t clilen;
	
	struct sockaddr_in serv_addr, cli_addr;

	pthread_t threads[10];	//threads for handling each process separately
	pthread_t access;

	if (argc < 2) {
		cout << "Port number is not provided\n";
		exit(1);
	}

	if(pthread_mutex_init(&lock1, NULL) != 0)		// Initializing mutex object
	{
		cout << "Queue mutex init has failed" << endl;
		return -1;
	}

	if(pthread_mutex_init(&lock2, NULL) != 0)		// Initializing mutex object
	{
		cout << "Conditional Variable mutex init has failed" << endl;
		return -2;
	}

	server = socket(AF_INET, SOCK_STREAM, 0);	//creating a socket
	
	if(server < 0) 
	{
		cout << "Error while opening the socket";
		return -3;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if(bind(server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)	//binding a socket
	{	
		cout << "Error while binding the socket";
		return -1;
	}

	if(listen(server, 10) < 0)	
	{
		cout << "Cannot listen"<<endl;
		return -3;
	}
	
	clilen = sizeof(cli_addr);

	cout << "Enter the total number of processes requesting for access: ";
	cin >> t;

	cout << "This server process is the Centralized Mutual Exclusion Co-ordinator! " << endl;

	int sockid = 0;

	for(int i=0; i < t; i++)
	{
		newserver[sockid] = accept(server,(struct sockaddr *) &cli_addr,&clilen);

		llist.push_back(newserver[sockid]);

		pthread_create(&threads[i], NULL, Connections, (void *)&newserver[sockid]);
		
		sockid= sockid + 1;
	}
	
	pthread_create(&access, NULL, AccessProvider, NULL);

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

	pthread_mutex_destroy(&lock1);
	pthread_mutex_destroy(&lock2);

	close(server);
	
	pthread_exit(NULL);
	return 0; 
}




void *Connections(void *newserver) 				// thread function for each client request
{
	int req, n;
	long *ptr = (long *)newserver;
	long sock = *ptr;
	if(sock < 0) 
	{
		cout << "Error while accepting the connection request";
		exit(0);
	}

	cout << "Connected to Process Id: " << sock << endl;
	
	while(1)
	{
		n = read(sock,&req,sizeof(int));		// Reading Client's Request
		if (n < 0) 
		{
			cout << "Error while reading from the socket";
			exit(0);
		}
		
		if(req==REQUEST)				// When the client is requesting access to shared file
		{
			cout << "File Access Request by Process " << sock << endl;
			pthread_mutex_lock(&lock1);
			que.push(sock);				// Queue the client request
			pthread_mutex_unlock(&lock1);
		}
		else if(req==RELEASE)				// When the client releases the file access
		{
			pthread_mutex_lock(&lock2);
			if(cs == 1)				// if critical section is acquired 
			{
				cs = 0;				// Release the critical section
				cout << "Access to the file is released by the process " << sock << endl;
				pthread_cond_signal(&cv);	// Signal the conditional variable
			}
			pthread_mutex_unlock(&lock2);
		}
		else if(req==FINISH)				// When client sends a finish message, break from the loop
		{
			cout << "Work Completed. Exiting --> Process " << sock << endl;
			break;
		}
	}

	for(list<long> :: iterator s = llist.begin(); s != llist.end(); )	// Remove the particular client from active client's list
	{
		if(*s == sock)
			s = llist.erase(s);
		else s++;
	}
	
	close(sock);
	pthread_exit(NULL);
}

void *AccessProvider(void *arg) 							// thread function for granting access to Shared File to each client
{
	long req_grant;
	int req, n;
	cout << "Access Co-ordinator is working" << endl;	
	while(1)
	{
		if(llist.empty())							// When there are no active clients, exit the thread
		{
			break;
		}
		pthread_mutex_lock(&lock2);
		if(cs == 1)							// When there is someone in the critical section
		{				
			pthread_cond_wait(&cv, &lock2);			// Wait on the conditional variable
		}
		pthread_mutex_lock(&lock1);
		if(!que.empty())							// When there are requests pending in the queue
		{
			cs = 1;							// Mark the critical section as taken 
			req_grant = que.front();				
			que.pop();						// Remove the request from queue
			req = OK;
			n = send(req_grant,&req,sizeof(int), 0);			// Sending Permission to Access
			if (n < 0) 
			{
				cout << "Error while writing to the socket";
				exit(0);
			}
			cout << "Permission Granted to the process " << req_grant << endl; 
		}
		pthread_mutex_unlock(&lock1);
		pthread_mutex_unlock(&lock2);
	}
	cout << "Access Coordinator's work is done!" << endl;			
}


