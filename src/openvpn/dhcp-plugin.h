#ifndef __DHCP_PLUGIN_H__
#define __DHCP_PLUGIN_H__

#include <stdint.h>

#define DNS_NAME_LEN 100
#define MAC_ADDR_LEN 6

struct dhcp_lease 
{    
    uint32_t client_ip;
    uint32_t mask_ip;
    uint32_t router_ip;
    uint32_t dns_ip_1; 
    uint32_t dns_ip_2;
    char dns[DNS_NAME_LEN];
    uint32_t lease_time;
    uint32_t renew_time;
    char addr[MAC_ADDR_LEN];
    bool status;
 //   dhcp_lease * next;
};

#endif /* __DHCP_PLUGIN_H__ */
