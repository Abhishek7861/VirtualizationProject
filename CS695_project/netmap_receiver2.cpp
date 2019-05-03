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
// struct netmap_ring *send_ring1, *receive_ring1;
// struct netmap_ring *send_ring2, *receive_ring2;
vector<netmap_ring *> send_ring, receive_ring;
// struct nm_desc *d1;
// struct nm_desc *d2;
vector<string> interface;
vector<thread> threads;
vector<nm_desc *> d;
struct nmreq nmr;
vector<pollfd> fds;
// struct pollfd fds1;
// struct pollfd fds2;
int fd, length;
mutex lockcount;
#define PACKET_SIZE 1024

void send_buffer(char *buffer,int i)
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
    struct wireBuffer wiremsg
    {
        udphdr.encode(), iphdr.encode(), msg
    };
    char *dst = NETMAP_BUF(send_ring[i], send_ring[i]->slot[send_ring[i]->cur].buf_idx);
    memcpy(dst,(wiremsg.encode()).c_str(),PACKET_SIZE);
    send_ring[i]->cur = nm_ring_next(send_ring[i], send_ring[i]->cur);
    send_ring[i]->head = send_ring[i]->cur;
    ioctl(fds[i].fd, NIOCTXSYNC, NULL);
}

// void send_buffer2(char *buffer)
// {
//     length = strlen(buffer);
//     string msg = "hello";
//     struct udp_header udphdr
//     {
//         1, 2, 3, 4
//     };

//     struct ip_header iphdr
//     {
//         1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
//     };
//     struct wireBuffer wiremsg
//     {
//         udphdr.encode(), iphdr.encode(), msg
//     };
//     char *dst = NETMAP_BUF(send_ring2, send_ring2->slot[send_ring2->cur].buf_idx);
//     memcpy(dst, (wiremsg.encode()).c_str(), (wiremsg.encode().length()));
//     send_ring2->cur = nm_ring_next(send_ring2, send_ring2->cur);
//     send_ring2->head = send_ring2->cur;
//     ioctl(fds2.fd, NIOCTXSYNC, NULL);
// }

void recvd(int i)
{
    char *src;
    int cur = receive_ring[i]->cur;
    int n, rx;

    int count = 0;
    while (1)
    {
        // lockcount.lock();
        poll(&fds[i], 1, -1);
        ioctl(fds[i].fd, NIOCRXSYNC, NULL);
        n = nm_ring_space(receive_ring[i]);
        for (rx = 0; rx < n; rx++)
        {
            struct netmap_slot *slot = &receive_ring[i]->slot[cur];
            src = NETMAP_BUF(receive_ring[i], slot->buf_idx);
            length = slot->len;
            cur = nm_ring_next(receive_ring[i], cur);
        }
        count = count + rx;
        send_buffer(src,i);
        receive_ring[i]->head = receive_ring[i]->cur = cur;
        send_ring[i]->head = send_ring[i]->cur;
        ioctl(fds[i].fd, NIOCTXSYNC, NULL);
        // lockcount.unlock();

        struct wireBuffer decodewire;
        decodewire.decode(string(src), &decodewire);
        // cout << i<<" ip = " << decodewire.ip << endl;
        // cout << i<< "msg = " << decodewire.msg << endl;
        // cout << i<< "udp = " << decodewire.udp << endl;
    }
}

// void recvd2()
// {
//     char *src;
//     int cur = receive_ring2->cur;
//     int n, rx;

//     int count = 0;
//     while (1)
//     {
//         lockcount.lock();
//         poll(&fds2, 1, -1);
//         ioctl(fds2.fd, NIOCRXSYNC, NULL);
//         n = nm_ring_space(receive_ring2);
//         for (rx = 0; rx < n; rx++)
//         {
//             struct netmap_slot *slot = &receive_ring2->slot[cur];
//             src = NETMAP_BUF(receive_ring2, slot->buf_idx);
//             length = slot->len;
//             cur = nm_ring_next(receive_ring2, cur);
//         }
//         count = count + rx;
//         send_buffer2(src);
//         receive_ring2->head = receive_ring2->cur = cur;
//         lockcount.unlock();

//         struct wireBuffer decodewire;
//         decodewire.decode(string(src), &decodewire);
//         cout << "2 ip = " << decodewire.ip << endl;
//         cout << "2 msg = " << decodewire.msg << endl;
//         cout << "2 udp = " << decodewire.udp << endl;
//     }
// }

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

    int n;
    int timer;
    cout << "whats the count of threads" << endl;
    cin >> n;
    // vector<thread> threads;

    for (int i = 0; i <= 100; i++)
        interface.push_back("vale:" + to_string(i));

    for (int i = 0; i < n; i++)
    {
         d.push_back(nm_open((interface[(2*i+2)]).c_str(), &base_req, 0, 0));
        struct pollfd fds1;
        fds1.fd = NETMAP_FD((d[i]));
        fds1.events = POLLIN;
        fds.push_back(fds1);
        receive_ring.push_back(NETMAP_RXRING((d[i])->nifp, 0));
        send_ring.push_back(NETMAP_TXRING((d[i])->nifp, 0));
        threads.push_back(thread(recvd,i));
    }
    for (auto &t : threads)
        t.join();
    for (int i = 0; i < n; i++)
        nm_close(d[i]);
}
