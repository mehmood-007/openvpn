#include "dhcpclientdl.h"

MODE mode;
static char* mac_to_str(unsigned char *ha)
{
    char macstr_buf[18] = {'\0', };
    sprintf(macstr_buf, "%02X:%02X:%02X:%02X:%02X:%02X", ha[0], ha[1], ha[2], ha[3], ha[4], ha[5]);
    return macstr_buf; 
}
static void release_dhcp_ip( char * addr, char * username, uint32_t client_ip )
{
  dhcp_release( addr, username, client_ip);   
}
void dhcp_client( char * username, char * dhcp_lease)
{
    mode = IPv4;
    daemon_dhcp = 1;
    
    portset = 0;
    srand(time(NULL));
    struct interface * config_interface = malloc(sizeof(struct interface));

    memset( config_interface, 0, sizeof(struct interface) );
    strcpy( config_interface->name, username );
    generate_mac( username, config_interface );
	
   // fprintf( err_dhcp, "network-interface is %s, macaddr=%s\n", network_interface->name, mac_to_str(network_interface->addr) );
   // fprintf( err_dhcp, "user-interface is %s, macaddr=%s\n", config_interface->name, mac_to_str(config_interface->addr) );

    init_dhcp( config_interface->addr, username, dhcp_lease );
}
