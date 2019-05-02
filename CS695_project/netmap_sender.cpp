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

#define ARP_REQUEST 1
#define ARP_REPLY 2
#define ARP_CACHE_LEN 200
#define BACKEND_SERVERS 1

struct netmap_if *nifp;
struct netmap_ring *send_ring, *receive_ring;
struct nm_desc *d;
struct nmreq nmr;
struct pollfd fds;
int fd, length;
int flag = 1;
mutex lockcount;
mutex sendrecvlock;
unsigned long long int globalcount=0;
void send_batch()
{
    send_ring->head = send_ring->cur;
    ioctl(fds.fd, NIOCTXSYNC, NULL);
}

void process_receive_buffer()
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
    char *dst = NETMAP_BUF(send_ring, send_ring->slot[send_ring->cur].buf_idx);
    memcpy(dst, (wiremsg.encode()).c_str(), (wiremsg.encode().length()));
    send_ring->cur = nm_ring_next(send_ring, send_ring->cur);
    send_ring->head = send_ring->cur;
    ioctl(fds.fd, NIOCTXSYNC, NULL);
}

void sendnrecv()
{

    unsigned long long int count=0;    
    while (flag)
    {
        sendrecvlock.lock();
        int cur = receive_ring->cur;
        int n, rx;
        char *src;
        process_receive_buffer();
        poll(&fds, 1, -1);
        ioctl(fds.fd, NIOCRXSYNC, NULL);
        n = nm_ring_space(receive_ring);
        for (rx = 0; rx < n; rx++)
        {
            struct netmap_slot *slot = &receive_ring->slot[cur];
            src = NETMAP_BUF(receive_ring, slot->buf_idx);
            length = slot->len;
            cur = nm_ring_next(receive_ring, cur);
        }
        sendrecvlock.unlock();

        struct wireBuffer decodewire;
        decodewire.decode(string(src), &decodewire);
        cout << "ip = " << decodewire.ip << endl;
        cout << "msg = " << decodewire.msg << endl;
        cout << "udp = " << decodewire.udp << endl;
        count++;
    }
    lockcount.lock();
    globalcount =globalcount+count;
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
    d = nm_open("vale:3", &base_req, 0, 0);
    fds.fd = NETMAP_FD(d);
    fds.events = POLLIN;
    receive_ring = NETMAP_RXRING(d->nifp, 0);
    std::cout << "number of slots" << receive_ring->num_slots << std::endl;
    std::cout << "buffer size" << receive_ring->nr_buf_size << std::endl;
    std::cout << "total buffer size" << base_req.nr_memsize << std::endl;
    std::cout << "rx slots, tx slots" << base_req.nr_rx_slots << " " << base_req.nr_tx_slots << std::endl;

    send_ring = NETMAP_TXRING(d->nifp, 0);

    int n;
    int timer;
    cout << "whats the count of threads" << endl;
    cin >> n;
    cout << "whats the time of loadtest" << endl;
    cin >> timer;
    vector<thread> threads;
    for (int i = 0; i < n; i++)
    {
        threads.push_back(thread(sendnrecv));
    }
    sleep(timer);
    flag = 0;
    for (auto &t : threads)
        t.join();
    cout << "Throughput = " << globalcount / timer << endl;
    // close(sockfd);
    // sendnrecv();
    nm_close(d);
}
