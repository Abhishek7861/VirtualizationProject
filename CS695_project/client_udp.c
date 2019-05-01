// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
using namespace std;


mutex lockcount; 
#define PORT 8080
#define MAXLINE 1024
char *hello = "Hello from client";
char buffer[MAXLINE];
struct sockaddr_in servaddr;
int sockfd;
int flag = 1;	
unsigned long long globalcount = 0;


void serviceThread()
{
	unsigned long count = 0;

	while (flag)
	{
		int n;
		socklen_t len;
		sendto(sockfd, (const char *)hello, strlen(hello),
			   MSG_CONFIRM, (const struct sockaddr *)&servaddr,
			   sizeof(servaddr));
		// printf("Hello message sent.\n");

		n = recvfrom(sockfd, (char *)buffer, MAXLINE,
					 MSG_WAITALL, (struct sockaddr *)&servaddr,
					 &len);
		buffer[n] = '\0';
		// printf("Server : %s\n", buffer);
		count++;
	}
	lockcount.lock();
	globalcount = globalcount+count;
	lockcount.unlock();

}

int main()
{
	int n;
	int timer;
	cout << "whats the count of threads" << endl;
	cin >> n;
	cout << "whats the time of loadtest" << endl;
	cin >> timer;

	vector<thread> threads;

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = INADDR_ANY;

	for (int i = 0; i < n; i++)
	{
		threads.push_back(thread(serviceThread));
	}
	sleep(timer);
	flag = 0;
	for (auto &t : threads)
		t.join();
	cout<<"Throughput = "<<globalcount/timer<<endl;
	close(sockfd);
	return 0;
}
