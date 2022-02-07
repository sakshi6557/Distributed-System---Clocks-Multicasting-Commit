#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <list>

using namespace std;

struct newserver{						// A structure for all the required information of individual process is maintained
	int process_id;
	long socket_fd;
	long port;
	struct sockaddr_in new_serv_addr;
	struct hostent *new_server;
};

int cnt = 1, tot_pro = 4, noc = 1;
struct newserver p[10];
int vclock[10], myProcID;
pthread_mutex_t lock;
list<string> q;

void *Connections(void *server);	//to connect processes
int CausalityCheck(string tmpstr);	//to check causality
int CheckBuffer();	//to check if the message in buffer can be delivered
void *MulticastRecv(void *server);	//separate method to handle by thread to receive the multicast message
void *MulticastSend(void *arg);	//separate method to handle by thread to send the multicast message

int main(int argc, char *argv[])
{

	int ans, n;
	long port;

	char buffer[256];
	long server, new_serv[10], portno;
	socklen_t clilen;
	
	struct sockaddr_in serv_addr, cli_addr;

	pthread_t mcastsend, mcastrecv, newconnections;		//threads for handling each process separately

	if (argc < 2) {
		fprintf(stderr,"ERROR, Port number is not provided\n");
		exit(1);
	}

	if(pthread_mutex_init(&lock, NULL) != 0)		//Initializing mutex object
	{
		cout << "mutex init has failed" << endl;
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
	
	int reuse = 1;
	if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)	//To reuse the socket address if it crashes/failures
		perror("setsockopt(SO_REUSEADDR) failed");

	#ifdef SO_REUSEPORT
	if (setsockopt(server, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
		perror("setsockopt(SO_REUSEPORT) failed");
	#endif

	if(bind(server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)	//binding a socket
	{
	cout << "Error while binding on the socket";
	return -1;
	}
	if(listen(server, 10) < 0)
	{
		cout << "Cannot listen"<<endl;
		return -3;
	}
	cout << "What is your process ID? ";
	cin >> myProcID;
	p[0].process_id = myProcID;
	p[0].port = portno;
	p[0].socket_fd = server;
	if(myProcID == 1){
		ans = 1;
		noc = 3;

	}
	if(myProcID == 2){
		ans = 1;
		noc = 2;
	}
	if(myProcID == 3){
		ans = 1;
		noc = 1;
	}
	if(myProcID == 4){
		ans = 2;
	}
	int rc = pthread_create(&newconnections, NULL, Connections, (void*)server);
	
	cout << "Establishing Connections" << endl;
	
	if(ans == 1)
	{
		for(int i=1; i <= noc; i++)
		{
			cout << "Enter the port number of process to connect(start from 4th process portno): ";
			cin >> p[i].port;
			p[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	
			if(p[i].socket_fd < 0) 
				cout << "Error while opening the socket";

			p[i].new_server = gethostbyname("localhost");
	
			bzero((char *) &p[i].new_serv_addr, sizeof(p[i].new_serv_addr));
			p[i].new_serv_addr.sin_family = AF_INET;
			bcopy((char *)p[i].new_server->h_addr, (char *)&p[i].new_serv_addr.sin_addr.s_addr, p[i].new_server->h_length);
			p[i].new_serv_addr.sin_port = htons(p[i].port);


			if(connect(p[i].socket_fd,(struct sockaddr *) &p[i].new_serv_addr,sizeof(p[i].new_serv_addr)) < 0) 
				{
				cout << "Error while connecting to the Process";
				return -1;
				}

			bzero(buffer,256);
			recv(p[i].socket_fd,buffer,sizeof(buffer), 0);			//Reading the machine ID of connected machines
			
			stringstream ss, ss1, ss2;
			ss << buffer;
			string tmpstr = ss.str();

			p[i].process_id = atoi(tmpstr.c_str());

			cout << "Connected to the Process: " << p[i].process_id << endl;
			
			bzero(buffer,256);
			
			ss1 << p[0].process_id;
			string tmpstr1 = ss1.str();				// Converting Process ID from int to string or char array
			strcpy(buffer,tmpstr1.c_str());				// Now converting from string to const char *
	
			n = send(p[i].socket_fd,buffer,strlen(buffer), 0);	// Sending This machine's process ID to connected machine
			if (n < 0) cout << "Error while writing to the socket";

			bzero(buffer,256);
     			n = recv(p[i].socket_fd,buffer,255, 0);

			bzero(buffer,256);

			ss2 << p[0].port;
			string tmpstr2 = ss2.str();				// Converting Port Number from long to string or char array
			strcpy(buffer,tmpstr2.c_str());				// Now converting from string to const char *
	
			n = send(p[i].socket_fd,buffer,strlen(buffer), 0);	// Sending This machine's Port Number to connected machine
			if (n < 0) cout << "Error while writing to the socket";

			bzero(buffer,256);
     			n = recv(p[i].socket_fd,buffer,255, 0);

			cnt++;
		}
		noc++;								// Incrementing the counter for any processes still accepting new connections
	}

	while(cnt < tot_pro)							// Waiting till all the processes are connected
	{
		continue;
	}

	cout << "---------------------------------------------" << endl;
	cout << "Connections Established" << endl;

	for(int i=0; i < (tot_pro-1); i++)						// Sorting the processes according to their ID values
	{
		for(int j =0; j < (tot_pro -i-1); j++)
		{
			if(p[j].process_id > p[j+1].process_id)
			{
				swap(p[j], p[j+1]);
			}
		}
	}
	
	for(int i=0; i < tot_pro; i++)							// Spawning Multicast Receive Threads
	{
		if(i == (myProcID - 1))
		{
			continue;
		}
		else
		{
			pthread_create(&mcastrecv, NULL, MulticastRecv, (void *)p[i].socket_fd);
		}
	}

	pthread_create(&mcastsend, NULL, MulticastSend, NULL);				// Spawning Multicast Send Thread

	while(1)
	{
		continue;
	}

	pthread_mutex_destroy(&lock);					// Code never reacher here, but still following good practice to destroy mutexes

}



void *Connections(void *server) 				// thread function for accepting new connection request
{
	int i = 0, n;
	char buffer[256];
	socklen_t clilen;
	long new_serv[10];
	struct sockaddr_in cli_addr;
	clilen = sizeof(cli_addr);

	struct newserver p1[10];

	while(cnt < tot_pro)					// Running the Accept loop till desired number of processes have connected
	{
		new_serv[i] = accept((long)server,(struct sockaddr *) &cli_addr,&clilen);

		bzero(buffer,256);			
		stringstream ss, ss1, ss2;
		ss << p[0].process_id;
		string tmpstr = ss.str();				// Converting Process ID from int to string or char array
		strcpy(buffer,tmpstr.c_str());				// Now converting from string to const char * and copying to buffer

		n = send(new_serv[i],buffer,strlen(buffer), 0);	// Sending This machine's process ID to connected machine
		if (n < 0)
		{
		 cout <<"Error while writing to the socket";
		 }

		bzero(buffer,256);
		recv(new_serv[i],buffer,255, 0);			// Reading the machine ID of connected machine
		
		ss1 << buffer;
		string tmpstr1 = ss1.str();
	
		p1[i].process_id = atoi(tmpstr1.c_str());

		cout << "Connected to the Process: <" << p1[i].process_id << ">, ";

		n = send(new_serv[i],"ID received",11, 0);
		if (n < 0) cout << "Error while writing to the socket";

		bzero(buffer,256);
		recv(new_serv[i],buffer,255, 0);			// Reading the port number of connected machine
		
		ss2 << buffer;
		string tmpstr2 = ss2.str();
	
		p1[i].port = atoi(tmpstr2.c_str());			// Saving the port number of connected machine

		cout << "with the port number <" << p1[i].port << ">" << endl;

		n = send(new_serv[i],"Port number received",13, 0);
		if (n < 0) cout << "Error while writing to the socket";

		p1[i].socket_fd = new_serv[i];				// Saving the Socket Descriptor of connected machine

		i++;							// counter for accepted connections
		cnt++;							// counter for total connections
	}

	for(int j=0; j < i; j++)					// Storing the accepted process details into main datastructure of all processes
	{
		p[noc] = p1[j];
		noc++;
	}
	
}

int CausalityCheck(string tmpstr)
{
	int tmpArray[10];					// Array for temporarily storing received vector clock
	int index;						// Integer to store ID of sender process
		
	stringstream ss1(tmpstr);

	for(int i=0; i < tot_pro; i++)			// Getting the sender's vector clock from buffer into a temporary vector clock array
	{
		ss1 >> tmpArray[i];
	}
			
	ss1 >> index;

	int flag = 1;					// Flag to check Causal Ordering of events

	for(int i=0; i < tot_pro; i++)
	{
		if(i == (index - 1))
		{
			if(tmpArray[i] != (vclock[i] + 1))	// Check for Causal Ordering Condition 1 violation
			{
				flag = 0;
				break;
			}
		}
		else
		{
			if(tmpArray[i] > (vclock[i]))	// Check for Causal Ordering Condition 2 violation
			{
				flag = 0;
				break;
			}
		}
	}
	if(flag == 1)
	{
		return index;
	}
	else
	{
		return -1;
	}
}

int CheckBuffer()
{
	int flag = 0;
	for(list<string> :: iterator s = q.begin(); s != q.end(); )
	{
		int result = CausalityCheck(*s);
		if(result > 0)							// If both conditions satisfied, message is delivered to application
		{
			flag = 1;
			cout << "\n*******Message Delivered from Queue for Process " << result << "*******" << endl;
		
			vclock[result - 1]++;


			cout << "Vector Clock: (";
			for(int i = 0; i < (tot_pro - 1); i++)
			{
				cout << "[" << vclock[i] << "]" << ",";
			}
			cout << "[" << vclock[tot_pro - 1] << "]" << ")\n";
		
			
			s = q.erase(s);
			break;

		}
		else
		{
			s++;
		}
	}
	return flag;
}

void *MulticastRecv(void *server) 					// thread function for receiving multicast message from particular process
{
	char buffer[256];
	int n;
	long socket_fd = (long)server;					// Socket FD for communicating with specific process
	
	while(1)
	{
		
		int tmpArray[10];					// Array for temporarily storing received vector clock
		int index;						// Integer to store ID of sender process

		bzero(buffer,256);		
		
		srand(time(0));
		sleep(rand()%5);
	
		int rc = recv(socket_fd,buffer,sizeof(buffer), 0);	// Receive message from sender with their vector clock
		
		if(rc < 0)
		{
			cout << "Error while reading from the socket";		// Print respective message if there's an error while receving the message
		}
		else
		{
			stringstream ss;
			string tmpstr;
			ss.str("");
			ss << buffer;
			
			tmpstr = ss.str();
		
			int flag = CausalityCheck(tmpstr);
				
			if(flag > 0)					// If both conditions satisfied, message is delivered to application
			{
				cout << "\n-----------Multicast Message Received from Process " << flag << "-----------" << endl;
			
				vclock[flag - 1]++;


				cout << "Vector Clock: (";
				for(int i = 0; i < (tot_pro - 1); i++)
				{
					cout << "[" << vclock[i] << "]" << ",";
				}
				cout << "[" << vclock[tot_pro - 1] << "]" << ")\n";
	
				
				pthread_mutex_lock(&lock);
				if(!q.empty())				 // If the buffer has some pending messages, check if they satisfy causality now!
				{
					int res;
					do {
						res = CheckBuffer();
					} while(res == 1);		// If a message from buffer is delivered, check for the causality of remaining messages in buffer 
						
				}
				pthread_mutex_unlock(&lock);
				
			} 
			else							
			{
				
				if(!q.empty())				// If a message fails causal conditions, check if there are pending messages in buffer. Check if they satisfy causality now!
				{
					pthread_mutex_lock(&lock);
					int res, flag = 0;
					do {
						res = CheckBuffer();
						if(res == 1)		// A message from buffer is delivered!
						{
							if(flag == 0)
							{
								int recheck = CausalityCheck(tmpstr);	// If message from buffer is delivered. Re-Check for causality of current message
								if(recheck > 0)				// If both conditions are satisfied, deliver current message to application
								{	
									flag = 1;
									cout << "\n-----------Multicast Message Received from Process " << recheck << "-----------" << endl;
									vclock[recheck - 1]++;
	
									cout << "Vector Clock: (";
									for(int i = 0; i < (tot_pro - 1); i++)
									{
										cout << "[" << vclock[i] << "]" << ",";
									}
									cout << "[" << vclock[tot_pro - 1] << "]" << ")\n";
								}
							}
						}
					} while(res == 1);
					pthread_mutex_unlock(&lock);
					if(flag == 0)
					{		
						cout << "\n#######Causality Violation! Message Buffered#######" << endl;
						pthread_mutex_lock(&lock);	
						q.push_back(tmpstr);			// When any condition is not satisfied, message is buffered
						pthread_mutex_unlock(&lock);
					}	
				}
				else
				{
					cout << "\n#######Causality Violation! Message Buffered#######" << endl;

					pthread_mutex_lock(&lock);
					q.push_back(tmpstr);				// When any condition is not satisfied, message is buffered
					pthread_mutex_unlock(&lock);
				}
			}

		}

	}
}


void *MulticastSend(void *arg) 							// thread function for sending multicast message
{
	char buffer[256];
	int n;
	while(1)
	{	
		int ans;
		cout << "Press 1 to multicast: ";
		cin >> ans;
		if(ans==1)
		{
			int k = 0;	
			while(k<50)	
			{
				k++;
				vclock[myProcID-1]++;					// Send message event corresponds to one vector clock increment


				for(int i=0; i < tot_pro; i++)
				{
					if(i == (myProcID - 1))
					{
						continue;
					}
					else
					{
		
						bzero(buffer,256);			
						stringstream ss;
						string tmpstr;
	
						ss.str("");
						ss << vclock[0];
	
						tmpstr = ss.str();				// Converting Process ID from int to string or char array
						strcpy(buffer,tmpstr.c_str());
	
						strcat(buffer," ");
	
						for(int j=1; j < tot_pro; j++)
						{
							ss.str("");
							ss << vclock[j];
	
							tmpstr = ss.str();			// Converting Process ID from int to string or char array
							strcat(buffer,tmpstr.c_str());		// Now converting from string to const char *
						
							strcat(buffer," ");
	
						}
						ss.str("");
						ss << myProcID;
	
						tmpstr = ss.str();				// Converting Process ID from int to string or char array
						strcat(buffer,tmpstr.c_str());					
	
						n = send(p[i].socket_fd,buffer,sizeof(buffer), 0);
						if (n < 0) cout << "Error writing to the socket";
					}
				}

				srand(time(0));
				sleep(rand()%5);
		
				cout << "\nMulticast Message Sent from Process " << myProcID << endl;
				cout << "Vector Clock: (";
				for(int i = 0; i < (tot_pro - 1); i++)
				{
					cout << "[" << vclock[i] << "]" << ",";
				}
				cout << "[" << vclock[tot_pro - 1] << "]" << ")\n";
	
			}
		}
	}
}

