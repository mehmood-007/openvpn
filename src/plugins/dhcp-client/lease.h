#ifndef __LEASE_H__
#define __LEASE_H__

#include <stdint.h>

#define DNS_NAME_LEN 100

struct lease {
    uint32_t server_ip;
	
    uint32_t client_ip;
	
    uint32_t mask_ip;
	
    uint32_t router_ip;
	
    uint32_t dns_ip_1;

    uint32_t dns_ip_2;
    
    char dns[DNS_NAME_LEN];
	
    uint32_t lease_time;
	
    uint32_t renew_time;
	
    uint16_t portset_index;
	
    uint16_t portset_mask;
};



void save_lease( struct lease* lease, char * addr,  char * username );

int load_lease(struct lease* lease, char * addr, char * username);

#define DEFAULT_LEASE_PATH "/var/lib/open-dhcp/"

#endif /* __LEASE_H__ */
