
//#include <net/if_arp.h>
#include "interface.h"

/* Future Use: IPV6 implementation */
static inline int value(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	return 0;
}
/* Future Use: IPV6 implementation */
int get_ipv6_address(struct interface * network_interface)
{
	FILE *fin = fopen("/proc/net/if_inet6", "r");
	if (!fin) {
		fprintf(err_dhcp, "Can't open /proc/net/if_inet6\n");
		return -1;
	}
	char buf[200] = {0};
	int target_scope;
	if (mode == IPv6)
		target_scope = 0;//global
	else
		target_scope = 0x20;//link

	while (fgets(buf, 200, fin)) {
		char ipv6addr[100] = {0};
		int id;
		int mask_len;
		int scope;
		int flag;
		char name[100] = {0};
		sscanf(buf, "%s %x %x %x %x %s", ipv6addr, &id, &mask_len, &scope, &flag, name);
		if (strcmp(name, network_interface->name) == 0 && scope == target_scope) {
			memset(&src, 0, sizeof(src));
			src.sin6_family = AF_INET6;
			int i;
			for (i = 0; i < 16; ++i) {
				src.sin6_addr.s6_addr[i] = (value(ipv6addr[i * 2]) << 4) + value(ipv6addr[i * 2 + 1]);
			}
			fclose(fin);
			return 0;
		}
	}
	fprintf(err_dhcp, "%s IPv6 address of %s not found!\n", target_scope == 0 ? "Global" : "Link", network_interface->name);
	fclose(fin);
	return -1;
}
/* future use: generate MAC if not the mac on designated interface */
void generate_mac( char * uq_if_name, struct interface * config_interface){
    char addr[6] = {0};
    addr[0] = rand() % 0xFF;
	addr[1] = rand() % 0xFF;
	addr[2] = rand() % 0xFF;
	addr[3] = rand() % 0xFF;
	addr[4] = rand() % 0xFF;
	addr[5] = rand() % 0xFF;
	
	memcpy(config_interface->addr, addr, 6);
}
/* future use: Register dhcp-server interface for sending raw packet */
void init_interfaces(char* if_name )
{    
    err_dhcp = stderr;
    struct if_nameindex * interfaces = if_nameindex(), *interface;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    char addr[6] = {0};

    for (interface = interfaces; interface && interface->if_index; interface++) 
    {
        struct ifreq ifopt ;		
        int valid_addr = 0;

        memset(&ifopt, 0, sizeof(ifopt));
        strcpy(ifopt.ifr_name, interface->if_name);
        if (ioctl(fd, SIOCGIFHWADDR, &ifopt) == -1) 
        {
            fprintf(err_dhcp, "Failed to get MAC address of %s\n", interface->if_name);
            valid_addr = 0;
	} 
        else 
        {
            memcpy(addr, ifopt.ifr_hwaddr.sa_data, 6);
            valid_addr = 1;
	}		
	if ( strcmp(interface->if_name, if_name) == 0 ) 
        {
            network_interface = malloc(sizeof(struct interface));
            memset(network_interface, 0, sizeof(struct interface));
            strcpy(network_interface->name, if_name);
            memcpy(network_interface->addr, addr, 6);
            fprintf(err_dhcp, "network-interface MAC Found name=%s\n", network_interface->name);
	}
    }
    if_freenameindex(interfaces);
    close(fd);                        
    if (!network_interface) 
    {
	fprintf(err_dhcp, "network-interface not found ! name=%s\n", network_interface->name);
	exit(1);
    }
    if ((mode == IPv6 || mode == DHCPv6) && strlen(local_addr) == 0) {
	if (get_ipv6_address(network_interface) < 0){
            exit(1);
        }
    }
}
