#include "socket.h"

int  
init_socket( void )
{
        int listen_fd;
	struct timeval timeout;
	timeout.tv_sec = RECV_TIMEOUT_SEC;
	timeout.tv_usec = 0;

	if ( mode == IPv4 ) 
        {
        if ((listen_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) 
        {
            perror("dhcp_socket/socket");
            exit(1);
        }
        int flag = 1;
        if (setsockopt ( listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof flag) < 0) 
        {
            perror ("dhcp_socket/setsockopt: SO_REUSEADDR");
            exit (1);
        }
        if (setsockopt ( listen_fd, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof flag) < 0) 
        {
            perror ("dhcp_socket/setsockopt: SO_BROADCAST");
            exit (1);
        }
        if (setsockopt ( listen_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof timeout) < 0) 
        {
            perror ("dhcp_socket/setsockopt: SO_BROADCAST");
            exit (1);
        }
        
	memset( &servaddr, 0, sizeof(servaddr) );
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
        servaddr.sin_port = htons( IPv4_CLIENT_PORT );
        if ( bind( listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr) ) == -1 ) 
        {
            fprintf(err_dhcp, "OVPN-DHCP socket_init(): bind error\n");
            exit(1);
        }
    }
    if ( mode == IPv6 ) 
    {
	if ((ipv6_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) 
        {
            fprintf(err_dhcp, "Failed to create ipv6 sockfd!\n");
            exit(1);
	}
        struct sockaddr_in6 my_addr;
        bzero(&my_addr, sizeof(my_addr));
        my_addr.sin6_family = PF_INET6;
        my_addr.sin6_port = htons(IPv6_CLIENT_PORT);
        my_addr.sin6_addr = in6addr_any;
        if ( bind(ipv6_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in6) ) < 0) 
        {
            fprintf(err_dhcp, "Failed to bind!\n");
        }
        send6_fd = socket(PF_INET6, SOCK_RAW, IPPROTO_RAW);
        if (send6_fd < 0) 
        {
            fprintf(err_dhcp, "Failed to create send6 socket.\n");
            exit(1);
        }
	setsockopt(send6_fd, SOL_SOCKET, SO_BINDTODEVICE, network_interface_name, strlen(network_interface_name));
    } 
    else if ( mode == DHCPv6 ) 
    {
	if ((ipv6_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) 
        {
            fprintf(err_dhcp, "Failed to create DHCPv6 sockfd!\n");
            exit(1);
	}
	struct sockaddr_in6 myAddr;
        bzero(&myAddr, sizeof(myAddr));
        myAddr.sin6_family = PF_INET6;
        myAddr.sin6_port = htons(DHCPv6_CLIENT_PORT);
        myAddr.sin6_addr = in6addr_any;
        if (bind(ipv6_fd, (struct sockaddr*) &myAddr, sizeof(struct sockaddr_in6)) < 0) 
        {
            fprintf(err_dhcp, "Failed to bind!\n");
	}
	if ((send6_fd = socket(PF_INET6, SOCK_RAW, IPPROTO_RAW)) < 0) 
        {
            fprintf(err_dhcp, "Failed to create send6 socket.\n");
            exit(1);
	}
        setsockopt(send6_fd, SOL_SOCKET, SO_BINDTODEVICE, network_interface_name, strlen(network_interface_name));
    }
    return listen_fd;
}
void 
init_dhcp_ip(char * ip)
{
   dhcp_ip_ = ip;
}
void 
free_socket( int listen_fd )
{
    if (mode == IPv4)
    {
        if (listen_fd) 
        {
            close(listen_fd);
            listen_fd = 0;
        }
    }
    else if (mode == IPv6 || mode == DHCPv6) 
    {
	if (ipv6_fd) 
        {
            close(ipv6_fd);
            ipv6_fd = 0;
	}
	if (send6_fd) 
        {
            close(send6_fd);
            send6_fd = 0;
	}
    }
}
static struct timeval base;

static void timeout_init()
{
        gettimeofday(&base, 0);
}

static int timeout_check()
{
	struct timeval cur;
	gettimeofday(&cur, 0);
	if (cur.tv_sec - base.tv_sec - 1 > RECV_TIMEOUT_SEC)
		return 1;
	return 0;
}

static uint16_t udpchecksum(char *iphead, char *udphead, int udplen, int type)
{
	udphead[6] = udphead[7] = 0;
	uint32_t checksum = 0;
	//fprintf(err_dhcp, "udp checksum is 0x%02x%02x\n", (uint8_t)udphead[6], (uint8_t)udphead[7]);
	if (type == 6)
	{
		struct udp6_psedoheader header;
		memcpy(header.srcaddr, iphead + 24, 16);
		memcpy(header.dstaddr, iphead + 8, 16);
		header.length = ntohs(udplen);
		header.zero1 = header.zero2 = 0;
		header.next_header = 0x11;
		uint16_t *hptr = (uint16_t*)&header;
		int hlen = sizeof(header);
		while (hlen > 0) {
			checksum += *(hptr++);
			hlen -= 2;
		}
	}
	else if (type == 4)
	{
		struct udp4_psedoheader header;
		memcpy((char*)&header.srcaddr, iphead + 12, 4);
		memcpy((char*)&header.dstaddr, iphead + 16, 4);
		header.zero = 0;
		header.protocol = 0x11;
		header.length = ntohs(udplen);
		uint16_t *hptr = (uint16_t*)&header;
		int hlen = sizeof(header);
		while (hlen > 0) {
			checksum += *(hptr++);
			hlen -= 2;
		}
	}	
	uint16_t *uptr = (uint16_t*)udphead;
	while (udplen > 1) {	
		checksum += *(uptr++);
		udplen -= 2;
	}
	if (udplen) {
		checksum += (*((uint8_t*)uptr)) ;
	}
	do {
		checksum = (checksum >> 16) + (checksum & 0xFFFF);
	} while (checksum != (checksum & 0xFFFF));
	uint16_t ans = checksum;
	return (ans == 0xFF)? 0xFF :ntohs(~ans);
}

static uint16_t checksum(uint16_t *addr, int len)
{
	int nleft = len;
	int sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= sizeof (uint16_t);
	}

	if (nleft == 1) {
		*(uint8_t *) (&answer) = *(uint8_t *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

void send_packet( char * packet, int len, int listen_fd )
{
    switch ( mode ) 
    {
    case IPv4:
        send_udp_ipv4( packet, len, listen_fd, 0);
        return;
    case IPv6:
        send_packet_ipv6( packet, len );
        return;
    case DHCPv6:
        send_packet_dhcpv6( packet, len );
        return;
    default:
        fprintf(err_dhcp, "send_packet : unknown mode!\n");
        exit(0);
    }
}
void 
send_udp_ipv4( char* packet, int len, int dhcp_socket, char* server_ip )
{
    struct sockaddr_in dhcp_to;
    memset( &dhcp_to, 0, sizeof(dhcp_to) );
    dhcp_to.sin_family = AF_INET;
    dhcp_to.sin_port = htons( IPv4_SERVER_PORT );
    char * dst_ip = server_ip ? server_ip : dhcp_ip_;
    fprintf(err_dhcp, "OVPN-DHCP: Destination-ip %s", dst_ip);
    inet_aton( dst_ip , &(dhcp_to.sin_addr.s_addr));
    if ( dhcp_socket == -1) 
    {
        perror("OVPN-DHCP: dhcp_socket/socket");
        exit(1);
    }        
    if ( sendto( dhcp_socket, packet, len , 0, (struct sockaddr *)&dhcp_to, sizeof(dhcp_to) ) < 0 )
    {
        fprintf(err_dhcp, "OVPN-DHCP: Failed to Send UDP Datagram!\n");
    }
}
void send_raw_packet_ipv4(char * packet, int len, int listen_fd)
{
    unsigned char dst_mac[6] = {[0 ... 5] = 0xFF};
    int dhcp_socket;
    
    dhcp_socket = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    
    if ( dhcp_socket == -1) 
    {
        perror("dhcp_socket/socket");
        exit(1);
    }
/*    int flag = 1;
    if (setsockopt ( dhcp_socket, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof flag) < 0) 
    {
        perror ("dhcp_socket/setsockopt: SO_BROADCAST");
        exit(1);
    }
*/            
    char send_buf[1024];
    memset(send_buf, 0, sizeof(send_buf) );
    memcpy(send_buf + 14 + 20 + 8, packet, len);
    struct udphdr *udp = (struct udphdr*)(send_buf + 14 + 20);
    udp->source = htons(IPv4_CLIENT_PORT);
    udp->dest = htons(IPv4_SERVER_PORT);
    udp->len = htons(len + 8);
    udp->check = 0;

    struct iphdr* ip = (struct iphdr*)(send_buf + 14);
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0x10;
    ip->tot_len = htons(len + 20 + 8);
    ip->id = 0;
    ip->frag_off = 0;
    ip->ttl = 128;
    ip->protocol = UDP;
    ip->check = 0;
    inet_aton("0.0.0.0", &(ip->saddr));
    inet_aton("255.255.255.255", &(ip->daddr));

    udp->check = htons(udpchecksum((char*)ip, (char*)udp, len + 8, 4));
    ip->check = checksum((uint16_t*)ip, 20);
    
    memcpy(send_buf, dst_mac, 6);
    memcpy(send_buf + 6, network_interface->addr, 6);
    send_buf[12] = 0x08;
    send_buf[13] = 0x00;

    int total_len = len + 14 + 20 + 8;	
    struct sockaddr_ll device;
    if ((device.sll_ifindex = if_nametoindex(network_interface->name)) == 0) 
    {
        fprintf(err_dhcp, "Failed to resolve the index of %s.\n", network_interface->name);
    }
    int ret = sendto( dhcp_socket, total_len, sizeof( send_buf ), 0, (struct sockaddr *)&device, sizeof(device) );  
    if ( ret < 0) 
    {
        fprintf(err_dhcp, "Failed to send ipv4 packet %s, ret_val = %d.\n", network_interface->name, ret );
    }     
    close(dhcp_socket);
    dhcp_socket = 0;
}
void 
release_send_udp_ipv4( char * packet, int len, char* s_id )
{
    int listen_fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    send_udp_ipv4( packet, len, listen_fd, s_id );
    close(listen_fd);
    listen_fd = -1;
}
/* Recv UDP DHCPv4 */
int 
recv_packet(char* packet, int max_len, int listen_fd)
{
	switch (mode){
	case IPv4:
		return recv_udp_packet_ipv4(packet, max_len, listen_fd);
	case IPv6:
		return recv_packet_ipv6(packet, max_len);
	case DHCPv6:
		return recv_packet_dhcpv6(packet, max_len);
	default:
		fprintf(err_dhcp, "recv_packet : unknown mode!\n");
		exit(0);
	}
}
int 
recv_udp_packet_ipv4( char* packet, int max_len, int listen_fd )
{
  char recv_buf[max_len];
  timeout_init();
  int len = recv( listen_fd, recv_buf, max_len, 0 );
  if (len < 0 || timeout_check()) 
  {
        fprintf(err_dhcp, "recv timeout!\n");
        return -1;
  }
  memcpy( packet, recv_buf, len );
  return len;
}
int 
recv_packet_ipv4(char* packet, int max_len, int listen_fd)
{
  char recv_buf[max_len];
  timeout_init();
  int len = recv( listen_raw_fd, recv_buf, max_len, 0 );
  if (len < 0 || timeout_check()) 
  {
        fprintf(err_dhcp, "recv timeout!\n");
	return -1;
  }
  len -= 14 + 20 + 8;
  memcpy(packet, recv_buf + 14 + 20 + 8, len);
  return len;
}
void 
send_packet_ipv6(char *packet, int len)
{
	memset(buf, 0, sizeof(buf));
	memcpy(buf + 40 + 8, packet, len);
	struct ip6_hdr *ip6hdr = (struct ip6_hdr *)buf;
	ip6hdr->ip6_flow = htonl((6 << 28) | (0 << 20) | 0);		
	ip6hdr->ip6_plen = htons(len + 8);
	ip6hdr->ip6_nxt = IPPROTO_UDP;
	ip6hdr->ip6_hops = 128;
	memcpy(&(ip6hdr->ip6_src), &(src.sin6_addr), sizeof(struct in6_addr));
	memcpy(&(ip6hdr->ip6_dst), &(dest.sin6_addr), sizeof(struct in6_addr));
	
	struct udphdr *udp = (struct udphdr*)(buf + 40);
	udp->source = htons(IPv6_CLIENT_PORT);
	udp->dest = htons(IPv6_SERVER_PORT);
	udp->len = htons(len + 8);
	udp->check = 0;
	udp->check = htons(udpchecksum((char*)ip6hdr, (char*)udp, len + 8, 6));

	if (sendto(send6_fd, buf, len + 40 + 8, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
		fprintf(err_dhcp, "Failed to send ipv6 packet.\n");
		exit(1);
	}
}
/* Send DHCPv4 over DHCPv6 */
void 
send_packet_dhcpv6(char* packet, int len) {
	struct ip6_hdr* hdr = (struct ip6_hdr*) buf;
	struct udphdr* udp = (struct udphdr*) (buf + 40);
	struct DHCPv6Header* dhcpv6Hdr = (struct DHCPv6Header*) (buf + 40 + 8);
	memset(buf, 0, sizeof(buf));
	memcpy(buf + 40 + 8 + sizeof(struct DHCPv6Header), packet, len);
	hdr->ip6_flow = htonl((6 << 28) | (0 << 20) | 0);
	hdr->ip6_plen = htons(len + 8 + sizeof(struct DHCPv6Header));
	hdr->ip6_nxt = IPPROTO_UDP;
	hdr->ip6_hops = 128;
	memcpy(&(hdr->ip6_src), &(src.sin6_addr), sizeof(struct in6_addr));
	memcpy(&(hdr->ip6_dst), &(dest.sin6_addr), sizeof(struct in6_addr));
	udp->source = htons(DHCPv6_CLIENT_PORT);
	udp->dest = htons(DHCPv6_SERVER_PORT);
	udp->len = htons(len + 8 + sizeof(struct DHCPv6Header));
	udp->check = 0;
	dhcpv6Hdr->msgType = BOOTREQUESTV6; //Message ID TBD
	//Debug info
	dhcpv6Hdr->transactionID[0] = 0;
	dhcpv6Hdr->transactionID[1] = 0;
	dhcpv6Hdr->transactionID[2] = 0;
	dhcpv6Hdr->optionCode = htons(OPTION_BOOTP_MSG); //Option code TBD
	dhcpv6Hdr->optionLen = htons(len);
	udp->check = htons(udpchecksum((char*) hdr, (char*) udp, len + 8 + sizeof(struct DHCPv6Header), 6));
	if (sendto(send6_fd, buf, len + 40 + 8 + sizeof(struct DHCPv6Header), 0, (struct sockaddr*) &dest, sizeof(dest)) < 0) {
		fprintf(err_dhcp, "Failed to send DHCPv6 packet.\n");
		exit(1);
	}
}
int 
recv_packet_ipv6(char* packet, int max_len)
{
    timeout_init();
    do {
        int len = recv(listen_raw_fd, buf, max_len, 0);
        if (len < 0 || timeout_check()) {
                fprintf(err_dhcp, "recv timeout!\n");
                return -1;
        }
        struct ip6_hdr *ip6hdr = (struct ip6_hdr *)(buf + 14);
        if (ip6hdr->ip6_flow != htonl((6 << 28) | (0 << 20) | 0))
                continue;
        if (ip6hdr->ip6_nxt != IPPROTO_UDP)
                continue;
        if (memcmp(&(ip6hdr->ip6_dst), &(src.sin6_addr), sizeof(struct in6_addr)) != 0)
                continue;
        struct udphdr *udp = (struct udphdr*)(buf + 14 + 40);
        if (udp->dest != htons(IPv6_CLIENT_PORT))
                continue;
        len -= 14 + 40 + 8;
        memcpy(packet, buf + 14 + 40 + 8, len);
        return len;
    } while (1);
    return -1;
}
int 
recv_packet_dhcpv6(char* packet, int max_len)
{
    timeout_init();
    do 
    {
        int len = recv(listen_raw_fd, buf, max_len, 0);
        if (len < 0 || timeout_check()) {
            fprintf(err_dhcp, "recv timeout!\n");
            return -1;
        }
        struct ip6_hdr *ip6hdr = (struct ip6_hdr *)(buf + 14);
        if (ip6hdr->ip6_flow != htonl((6 << 28) | (0 << 20) | 0))
            continue;
        if (ip6hdr->ip6_nxt != IPPROTO_UDP)
            continue;
        if (memcmp(&(ip6hdr->ip6_dst), &(src.sin6_addr), sizeof(struct in6_addr)) != 0)
            continue;
        struct udphdr *udp = (struct udphdr*)(buf + 14 + 40);
        if (udp->dest != htons(DHCPv6_CLIENT_PORT))
            continue;
        unsigned char *p = buf + 14 + 40 + 8;
        if (*p != BOOTREPLYV6)
                continue;
        p += 4;
        while (p < (unsigned char*)buf + len) 
        {
            if (*(unsigned short*)p == htons(OPTION_BOOTP_MSG)) 
            {
                p += 2;
                unsigned length = htons(*(unsigned short*)p);
                p += 2;
                memcpy(packet, p, length);//TODO : check
                return length;
            }
            p += 2;
            p += htons(*(unsigned short*)p) + 2;
        }
    } while (1);
    return -1;
}

