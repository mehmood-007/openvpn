#ifndef __DHCP_H__
#define __DHCP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "interface.h"
#include "lease.h"
#include "socket.h"

#define HOSTNAME_LEN 80

char hostname[HOSTNAME_LEN];
uint32_t server_id;

struct dhcp_packet {

    uint8_t op;

    uint8_t htype;

    uint8_t hlen;

    uint8_t hops;

    uint32_t xid;

    uint16_t secs;

    uint16_t flags;

    uint32_t ciaddr;

    uint32_t yiaddr;

    uint32_t siaddr;

    uint32_t giaddr;

    uint8_t chaddr[16];

    char sname[64];

    char file[128];

    uint8_t options[1500];

};
struct dhcp_lease {

    uint32_t client_ip;

    uint32_t mask_ip;
    
    uint32_t router_ip;
    
    uint32_t dns_ip_1;
    
    uint32_t dns_ip_2;
    
    char dns[DNS_NAME_LEN];
    
    uint32_t lease_time;
    
    uint32_t renew_time;

    char addr[6];

    bool status; 
};

typedef enum {
    DISCOVER,

    OFFER,
    
    REQUEST,
    
    ACK,
    
    RELEASE

} STATE;


struct dhcp_profile
{
    uint32_t xid; /* transaction ID */
    
    STATE next_state; /* states of socket */
    
    int timeout_count; /* Time Count for DHCP */
    
    int renew; /* whether renewing */
    
    int listen_fd;
    
    struct lease offer_lease;

    struct lease ack_lease;

};

#define PACKET_END(p) (((uint8_t*)(p)) + sizeof(struct dhcp_packet))
#define PACKET_INSIDE(i,p) (((uint8_t*)(i)) >= ((uint8_t*)(p)) && ((uint8_t*)(i)) <= PACKET_END(p))

void init_dhcp(char * mac_addr, char * username, char * dhcp_prof );

int handle_dhcp( char * mac_addr, char * username, char * dhcp_prof, struct dhcp_profile * dhcp_profile_t );

void generate_xid( uint32_t * xid );

void dhcp_discover( char * mac_addr, char * username, char * dhcp_lease, struct dhcp_profile * dhcp_profile_t);

void dhcp_offer( char * addr, char * username, char * dhcp_lease , struct dhcp_profile * dhcp_profile_t );

void dhcp_ack( char * addr, char * username, char * dhcp_lease, struct dhcp_profile * dhcp_profile_t );

void dhcp_release( char * mac_addr, char * username ,uint32_t ip  );

void process_lease( struct lease * lease, struct dhcp_packet * packet );

int check_packet( struct dhcp_packet *packet, char * addr, uint32_t xid );

int gen_options( struct dhcp_packet *packet, char * username, char * addr, STATE next_state, uint32_t client_ip, uint32_t server_ip );

int gen_option_message_type( uint8_t *options, int pos , STATE next_state );

int gen_option_host_name( uint8_t *options, int pos, char * hostname );

int gen_option_parameter_request_list( uint8_t *options, int pos );

int gen_option_server_id( uint8_t *options, int pos, uint32_t server_ip );

int gen_option_ip_address( uint8_t *options, int pos, uint32_t client_ip );

int gen_option_portset( uint8_t *options, int pos );

int add_option_least_time( uint8_t * options, int pos );

void create_dhcp_profile( struct dhcp_lease * dhcp_profile, struct lease lease, char * addr );

int gen_option_client_identifier( uint8_t * options, int pos, char * addr );

int portset;

extern FILE * err_dhcp;

#define TIMEOUT_RETRY_TIMES 0
#define DISCOVER 1
#define REQUEST 3
#define RELEASE 7
#define BOOT_REQUEST    1
#define BOOT_REPLY      2
#define BOOTREQUESTV6              20
#define BOOTREPLYV6                21
#define OPTION_PAD                   0
#define OPTION_SUBNETMASK            1
#define OPTION_ROUTER                3
#define OPTION_DNSSERVER             6
#define OPTION_HOSTNAME             12
#define OPTION_DOMAINNAME           15
#define OPTION_BROADCAST            28
#define OPTION_IPADDRESS            50
#define OPTION_LEASETIME            51
#define OPTION_MESSAGETYPE          53
#define OPTION_SERVERID             54
#define OPTION_CLIENTID             61
#define OPTION_PARAMETERREQUESTLIST 55
#define OPTION_RENEWALTIME          58
#define OPTION_PORTSET              224
#define OPTION_BOOTP_MSG            87
#define DNS_LENTH_SECONDARY         8
#define BCAST_FLAG                0x8000
#define ETH_FLAG                    1
#endif /* __DHCP_H__ */
