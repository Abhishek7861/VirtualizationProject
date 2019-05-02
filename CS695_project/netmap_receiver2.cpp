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
#include "serialDserial.h"
#include <thread>
#include <mutex>


#define ARP_REQUEST 1
#define ARP_REPLY 2
#define ARP_CACHE_LEN 200
#define BACKEND_SERVERS 1

struct netmap_if *nifp;
struct netmap_ring *send_ring1, *receive_ring1;
struct netmap_ring *send_ring2, *receive_ring2;
struct nm_desc *d1;
struct nm_desc *d2;
struct nmreq nmr;
struct pollfd fds1;
struct pollfd fds2;
int fd, length;
mutex lockcount;





void send_buffer1(char *buffer)
{
    length = strlen(buffer);
    string msg = "hello";
    struct udp_header udphdr
    {
        1, 2, 3, 4
    };

    struct ip_header iphdr
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
    };
    struct wireBuffer wiremsg{udphdr.encode(),iphdr.encode(),msg};
    char *dst = NETMAP_BUF(send_ring1, send_ring1->slot[send_ring1->cur].buf_idx);
    memcpy(dst, (wiremsg.encode()).c_str(), (wiremsg.encode().length()));
    send_ring1->cur = nm_ring_next(send_ring1, send_ring1->cur);
    send_ring1->head = send_ring1->cur;
    ioctl(fds1.fd, NIOCTXSYNC, NULL);
}

void send_buffer2(char *buffer)
{
    length = strlen(buffer);
    string msg = "hello";
    struct udp_header udphdr
    {
        1, 2, 3, 4
    };

    struct ip_header iphdr
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
    };
    struct wireBuffer wiremsg{udphdr.encode(),iphdr.encode(),msg};
    char *dst = NETMAP_BUF(send_ring2, send_ring2->slot[send_ring2->cur].buf_idx);
    memcpy(dst, (wiremsg.encode()).c_str(), (wiremsg.encode().length()));
    send_ring2->cur = nm_ring_next(send_ring2, send_ring2->cur);
    send_ring2->head = send_ring2->cur;
    ioctl(fds2.fd, NIOCTXSYNC, NULL);
}



void recvd1()
{
    char *src;
    int cur = receive_ring1->cur;
    int n, rx;

    int count = 0;
    while (1)
    {
        lockcount.lock();
        poll(&fds1, 1, -1);
        ioctl(fds1.fd, NIOCRXSYNC, NULL);
        n = nm_ring_space(receive_ring1);
        for (rx = 0; rx < n; rx++)
        {
            struct netmap_slot *slot = &receive_ring1->slot[cur];
            src = NETMAP_BUF(receive_ring1, slot->buf_idx);
            length = slot->len;
            cur = nm_ring_next(receive_ring1, cur);
        }
        count = count + rx;
        send_buffer1(src);
        receive_ring1->head = receive_ring1->cur = cur;
        lockcount.unlock();

        struct wireBuffer decodewire;
        decodewire.decode(string(src), &decodewire);
        cout << "1 ip = " << decodewire.ip << endl;
        cout << "1 msg = " << decodewire.msg << endl;
        cout << "1 udp = " << decodewire.udp << endl;
    }
}


void recvd2()
{
    char *src;
    int cur = receive_ring2->cur;
    int n, rx;

    int count = 0;
    while (1)
    {
        lockcount.lock();
        poll(&fds2, 1, -1);
        ioctl(fds2.fd, NIOCRXSYNC, NULL);
        n = nm_ring_space(receive_ring2);
        for (rx = 0; rx < n; rx++)
        {
            struct netmap_slot *slot = &receive_ring2->slot[cur];
            src = NETMAP_BUF(receive_ring2, slot->buf_idx);
            length = slot->len;
            cur = nm_ring_next(receive_ring2, cur);
        }
        count = count + rx;
        send_buffer2(src);
        receive_ring2->head = receive_ring2->cur = cur;
        lockcount.unlock();

        struct wireBuffer decodewire;
        decodewire.decode(string(src), &decodewire);
        cout << "2 ip = " << decodewire.ip << endl;
        cout << "2 msg = " << decodewire.msg << endl;
        cout << "2 udp = " << decodewire.udp << endl;
    }
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
    d1 = nm_open("vale:4", &base_req, 0, 0);
    d2 = nm_open("vale:2", &base_req, 0, 0);
    fds1.fd = NETMAP_FD(d1);
    fds2.fd = NETMAP_FD(d2);
    fds1.events = POLLIN;
    fds2.events = POLLIN;
    receive_ring1 = NETMAP_RXRING(d1->nifp, 0);
    receive_ring2 = NETMAP_RXRING(d2->nifp, 0);
    std::cout << "number of slots" << receive_ring1->num_slots << std::endl;
    std::cout << "buffer size" << receive_ring1->nr_buf_size << std::endl;
    std::cout << "total buffer size" << base_req.nr_memsize << std::endl;
    std::cout << "rx slots, tx slots" << base_req.nr_rx_slots << " " << base_req.nr_tx_slots << std::endl;

    std::cout << "number of slots" << receive_ring2->num_slots << std::endl;
    std::cout << "buffer size" << receive_ring2->nr_buf_size << std::endl;
    std::cout << "total buffer size" << base_req.nr_memsize << std::endl;
    std::cout << "rx slots, tx slots" << base_req.nr_rx_slots << " " << base_req.nr_tx_slots << std::endl;

    send_ring1 = NETMAP_TXRING(d1->nifp, 0);
    send_ring2 = NETMAP_TXRING(d2->nifp, 0);
    thread th1(recvd1);
    thread th2(recvd2);
    th1.join();
    th2.join();
    
    // int n;
    // int timer;
    // cout << "whats the count of threads" << endl;
    // cin >> n;
    // vector<thread> threads;
    // for (int i = 0; i < n; i++)
    // {
    //     threads.push_back(thread(recvd));
    // }
    // for (auto &t : threads)
    //     t.join();
    // nm_close(d);
}
