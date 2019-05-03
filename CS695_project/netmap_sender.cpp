#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>
#include <net/netmap.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;
#include <thread>
#include "serialDserial.h"
#include <mutex>
#include <vector>

#define ARP_REQUEST 1
#define ARP_REPLY 2
#define ARP_CACHE_LEN 200
#define BACKEND_SERVERS 1

struct netmap_if *nifp;
// struct netmap_ring *send_ring2, *receive_ring2;
// struct netmap_ring *send_ring1, *receive_ring1;
vector<netmap_ring*> send_ring, receive_ring;
// struct nm_desc *d1;
// struct nm_desc *d2;
vector<nm_desc*> d;
vector<string> interface;
vector<thread> threads;

struct nmreq nmr;
// struct pollfd fds1;
// struct pollfd fds2;
vector<pollfd> fds;
int fd, length;
int flag = 1;
mutex lockcount;
mutex sendrecvlock;
unsigned long long int globalcount = 0, globaltime=0;
#define PACKET_SIZE 1024

long microsecond1, microsecond2, ns;

void process_receive_buffer(int i)
{

    // length = strlen(buffer);
    string msg = "hello";
    struct udp_header udphdr
    {
        1, 2, 3, 4
    };

    struct ip_header iphdr
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
    };
    struct wireBuffer wiremsg
    {
        udphdr.encode(), iphdr.encode(), msg
    };
    char *dst = NETMAP_BUF((send_ring[i]), (send_ring[i])->slot[(send_ring[i])->cur].buf_idx);
    memcpy(dst, (wiremsg.encode()).c_str(),PACKET_SIZE);
    (send_ring[i])->cur = nm_ring_next((send_ring[i]), (send_ring[i])->cur);
    (send_ring[i])->head = (send_ring[i])->cur;
    ioctl(fds[i].fd, NIOCTXSYNC, NULL);
}

void sendnrecv(int i)
{

    unsigned long long int count = 0;
    struct timespec start, finish;
    while (flag)
    {
        // sendrecvlock.lock();
        int cur = receive_ring[i]->cur;
        int n, rx;
        char *src;
        clock_gettime(CLOCK_REALTIME, &start);
		microsecond1 = microsecond1 + 1000000000 * start.tv_sec + start.tv_nsec;
        process_receive_buffer(i);
        n = nm_ring_space(receive_ring[i]);
        for (rx = 0; rx < n; rx++)
        {
            struct netmap_slot *slot = &receive_ring[i]->slot[cur];
            src = NETMAP_BUF(receive_ring[i], slot->buf_idx);
            length = slot->len;
            cur = nm_ring_next(receive_ring[i], cur);
        }
        // sendrecvlock.unlock();
        receive_ring[i]->head = receive_ring[i]->cur = cur;
        send_ring[i]->head = send_ring[i]->cur;
        ioctl(fds[i].fd, NIOCRXSYNC, NULL); 
        // struct wireBuffer decodewire;
        // decodewire.decode(string(src), &decodewire);
        // cout <<i <<" ip = " << decodewire.ip << endl;
        // cout <<i <<" msg = " << decodewire.msg << endl;
        // cout << i<<" udp = " << decodewire.udp << endl;
        clock_gettime(CLOCK_REALTIME, &finish);

		microsecond2 = microsecond2 + 1000000000 * finish.tv_sec + finish.tv_nsec;
        count++;
    }
    lockcount.lock();
    globaltime = globaltime+(microsecond2-microsecond1);
    globalcount = globalcount + count;
    lockcount.unlock();
}



int main()
{
    char *buf;
    struct nm_pkthdr h;
    struct nmreq base_req, nmr;
    uint16_t *spkt, *dpkt;
    struct ether_addr *p;
    uint32_t source_ip;
    struct arp_cache_entry *entry;
    memset(&base_req, 0, sizeof(base_req));
    base_req.nr_flags |= NR_ACCEPT_VNET_HDR;
    // d1 = nm_open("vale:1", &base_req, 0, 0);
    // d2 = nm_open("vale:3", &base_req, 0, 0);
    // fds1.fd = NETMAP_FD(d1);
    // fds1.events = POLLIN;
    // fds2.fd = NETMAP_FD(d2);
    // fds2.events = POLLIN;
    // receive_ring1 = NETMAP_RXRING(d1->nifp, 0);
    // receive_ring2 = NETMAP_RXRING(d2->nifp, 0);
    // std::cout << "number of slots" << receive_ring1->num_slots << std::endl;
    // std::cout << "buffer size" << receive_ring1->nr_buf_size << std::endl;
    // std::cout << "total buffer size" << base_req.nr_memsize << std::endl;
    // std::cout << "rx slots, tx slots" << base_req.nr_rx_slots << " " << base_req.nr_tx_slots << std::endl;

    // std::cout << "number of slots" << receive_ring2->num_slots << std::endl;
    // std::cout << "buffer size" << receive_ring2->nr_buf_size << std::endl;
    // std::cout << "total buffer size" << base_req.nr_memsize << std::endl;
    // std::cout << "rx slots, tx slots" << base_req.nr_rx_slots << " " << base_req.nr_tx_slots << std::endl;

    // send_ring1 = NETMAP_TXRING(d1->nifp, 0);
    // send_ring2 = NETMAP_TXRING(d2->nifp, 0);

    int n;
    int timer;
    cout << "whats the count of threads" << endl;
    cin >> n;
    cout << "whats the time of loadtest" << endl;
    cin >> timer;
    for (int i = 0; i <= 100; i++)
        interface.push_back("vale:" + to_string(i));

    for (int i = 0; i < n; i++)
    {
        d.push_back(nm_open((interface[(2*i)+1]).c_str(), &base_req, 0, 0));
        struct pollfd fds1;
        fds1.fd = NETMAP_FD((d[i]));
        fds1.events = POLLIN;
        fds.push_back(fds1);
        receive_ring.push_back(NETMAP_RXRING((d[i])->nifp, 0));
        send_ring.push_back(NETMAP_TXRING((d[i])->nifp, 0));
        threads.push_back(thread(sendnrecv,i));
    }
    sleep(timer);
    flag = 0;
    for (auto &t : threads)
        t.join();
    cout << "Throughput = " << globalcount / timer << endl;
    // cout<<"Average Response time = " << ((double)globaltime/((double)globalcount)) << " ns" <<endl;
    for (int i = 0; i < n; i++)
        nm_close(d[i]);
}
