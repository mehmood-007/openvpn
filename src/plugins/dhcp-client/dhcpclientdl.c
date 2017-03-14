#include "dhcpclientdl.h"

MODE mode;
/* hex hw-addr to char */
static char* 
mac_to_str(unsigned char *ha)
{
    char macstr_buf[18] = {'\0', };
    sprintf(macstr_buf, "%02X:%02X:%02X:%02X:%02X:%02X", ha[0], ha[1], ha[2], ha[3], ha[4], ha[5]);
    return macstr_buf; 
}
/* calling dhcp-release */
static void 
release_dhcp_ip( char * addr, char * username, uint32_t client_ip )
{
  dhcp_release( addr, username, client_ip);   
}
/* main func for dhcp-client which behave as a bridge between openvn-core and dhcp-client */
void 
dhcp_client( char * username, char * dhcp_lease)
{
    /* As currently we support IPv4, but in future we can suppport IPv6 as well*/
    mode = IPv4;
    
    daemon_dhcp = 1;

    /* creating pseudo-random hex to create unique random-mac-address */
    srand(time(NULL));

    /* allocating resources for struct interface*/
    struct interface * config_interface = malloc(sizeof(struct interface));

    memset( config_interface, 0, sizeof(struct interface) );
   /* set username as an interface-name as dhcp-client will serve openvpn-client */
    strcpy( config_interface->name, username );

   /* generating random mac-addr based on the username */
    generate_mac( username, config_interface );

   /* staring the dhcp client-server communication */ 	 
    init_dhcp( config_interface->addr, username, dhcp_lease );
    free(config_interface);
}
