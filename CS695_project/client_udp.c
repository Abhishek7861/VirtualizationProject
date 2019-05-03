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
#define MAXLINE 2048
char *hello = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";
char buffer[MAXLINE];
struct sockaddr_in servaddr;
int sockfd;
int flag = 1;
unsigned long long globalcount = 0,globaltime=0;

void serviceThread()
{
	unsigned long count = 0;
	struct timespec start, finish;
	long microsecond1, microsecond2, ns;
	double total = 0;

	while (flag)
	{
		int n;
		socklen_t len;
		clock_gettime(CLOCK_REALTIME, &start);
		microsecond1 = 1000000000 * start.tv_sec + start.tv_nsec;
		sendto(sockfd, (const char *)hello, strlen(hello),
			   MSG_CONFIRM, (const struct sockaddr *)&servaddr,
			   sizeof(servaddr));
		// printf("Hello message sent.\n");

		n = recvfrom(sockfd, (char *)buffer, MAXLINE,
					 MSG_WAITALL, (struct sockaddr *)&servaddr,
					 &len);
		clock_gettime(CLOCK_REALTIME, &finish);

		microsecond2 = 1000000000 * finish.tv_sec + finish.tv_nsec;
		buffer[n] = '\0';
		// printf("Server : %s\n", buffer);

		lockcount.lock();
		globaltime = globaltime+(microsecond2-microsecond1);
		globalcount++;
		lockcount.unlock();
	}
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
		t.detach();
	cout << "Throughput = " << globalcount / timer << endl;
	cout<<"Average Response time = " << ((double)globaltime/((double)globalcount)) << " ns" <<endl;
	close(sockfd);
	return 0;
}
