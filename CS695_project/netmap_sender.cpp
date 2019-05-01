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

struct udp_hdr{
    u_int16_t source_port;
    u_int16_t dest_port;
    u_int16_t length;
    u_int16_t checksum;
};

struct ip_hdr{
    u_int8_t ver:4;
    u_int8_t lrngth:4;
    udp_hdr udpmsg;
};

struct msg{
    ip_hdr ip_msg;
};


void send_batch() {
	send_ring->head = send_ring->cur;
  	//std::cout << "T.Head: " << send_ring->head << std::endl;
  	//std::cout << "T.Tail: " << send_ring->tail << std::endl<< std::endl;
  	ioctl(fds.fd, NIOCTXSYNC, NULL);
}


void process_receive_buffer( char *buffer)
{
    // struct ip_hdr *ip_header = (struct ip_hdr *)buffer;
    // struct udp_hdr *udp_header =  (struct udp_hdr *)buffer+sizeof(ip_hdr);
    struct msg *send_msg = (struct msg *)buffer;
    length = strlen(buffer);
    send_msg->ip_msg.lrngth = 5;
    send_msg->ip_msg.ver = 4;
    send_msg->ip_msg.udpmsg.checksum = 12;
    send_msg->ip_msg.udpmsg.dest_port = 12;
    send_msg->ip_msg.udpmsg.length = 12;
    send_msg->ip_msg.udpmsg.source_port = 12;
    char *dst = NETMAP_BUF(send_ring, send_ring->slot[send_ring->cur].buf_idx);
    memcpy(dst, buffer, length);
    send_ring->cur = nm_ring_next(send_ring, send_ring->cur);
    send_ring->head = send_ring->cur;
    ioctl(fds.fd, NIOCTXSYNC, NULL);
}





int main()
{
    char *buf;
    struct nm_pkthdr h;
    struct nmreq base_req, nmr;
    // char src[20];
    // for(int i=0;i<20;i++)
    //     src[i] = 'h';
    char buffer[2048];
    for(int i=0;i<2047;i++)
    buffer[i] = 'h';
    buffer[2047] = '\n';
    char *src  ;
    char *sendstr =  (char *)malloc(sizeof(msg));
    // memcpy(src,buffer,2048);
       uint16_t *spkt, *dpkt;
    struct ether_addr *p;
    uint32_t source_ip;
    struct arp_cache_entry *entry;
//     arp_init();
  	// src_byte = ether_aton_src(src_mac);
    //dest_byte = ether_aton_dst(dest_mac);
    memset(&base_req, 0, sizeof(base_req));
    base_req.nr_flags |= NR_ACCEPT_VNET_HDR;
    //base_req.nr_rx_slots = 512;
    d = nm_open("vale:y", &base_req, 0, 0);
    fds.fd = NETMAP_FD(d);
    fds.events = POLLIN;
    receive_ring = NETMAP_RXRING(d->nifp, 0);
    /*struct	netmap_if *nifp;
   	fd = open("/dev/netmap", O_RDWR);
	 bzero(&base_req, sizeof(base_req));
	 strcpy(base_req.nr_name, "eth6");
	 base_req.nr_rx_slots = 2048;
	 ioctl(fd, NIOCREGIF, &base_req);
	 void *pq = mmap(0, base_req.nr_memsize, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	 nifp =	NETMAP_IF(pq, base_req.nr_offset);
	 receive_ring =	NETMAP_RXRING(nifp, 0);
	 send_ring = NETMAP_TXRING(nifp, 0);
	 fds.fd	= fd;
	 fds.events = POLLIN;*/
    std::cout << "number of slots" << receive_ring->num_slots << std::endl;
    std::cout << "buffer size" << receive_ring->nr_buf_size << std::endl;
    std::cout << "total buffer size" << base_req.nr_memsize << std::endl;
     std::cout << "rx slots, tx slots" << base_req.nr_rx_slots << " " << base_req.nr_tx_slots << std::endl;

    send_ring = NETMAP_TXRING(d->nifp, 0);
    // signal(SIGINT, sigint_h);

    int cur = receive_ring->cur;
    int n, rx;
    // for(int i=1;i>0;i++)
    while(1)
    process_receive_buffer(sendstr);
    // send_batch();
    // while (1) {
    //     //ioctl(fds.fd, NIOCRXSYNC, NULL);
    //     n = nm_ring_space(receive_ring);
    //     for(rx = 0; rx < n; rx++) {
    //     	struct netmap_slot *slot = &receive_ring->slot[cur];
    //         src = NETMAP_BUF(receive_ring, slot->buf_idx);
    //         length = slot->len;
    //         process_receive_buffer(src);
    //         cur = nm_ring_next(receive_ring, cur);
    //     }
    //     puts(src);
    //     poll(&fds,  1, 2);
    //     std::cout << "received" << rx << std::endl;
    //     //std::cout << "head" << receive_ring->head << std::endl;
    //     //std::cout << "tail" << receive_ring->cur << std::endl; 
    //     receive_ring->head = receive_ring->cur = cur;
    //     send_batch();
    // }
    nm_close(d);
    //close(fd);
}