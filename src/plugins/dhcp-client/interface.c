
//#include <net/if_arp.h>
#include "interface.h"

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
void init_interfaces(char * if_name )
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
static int check_dns_name(struct lease* lease)
{
	if (strlen(lease->dns) == 0)
		return 0;
	if (strcmp(lease->dns, "lan") == 0)
		return 0;
	return 1;
}



void configure_interface(struct lease* lease, char * hw_addr, char * user_name)
{
	struct sockaddr_in addr;
	struct sockaddr_in mask;
	struct sockaddr_in gateway;
	struct sockaddr_in dns;
	
        memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = lease->client_ip;
	
        memset(&mask, 0, sizeof(mask));
	mask.sin_family = AF_INET;
	mask.sin_addr.s_addr = lease->mask_ip;
	
        memset(&gateway, 0, sizeof(gateway));
	gateway.sin_family = AF_INET;
	gateway.sin_addr.s_addr = lease->router_ip;
	
        memset(&dns, 0, sizeof(dns));
	dns.sin_family = AF_INET;
	dns.sin_addr.s_addr = lease->dns_ip_1;
	
	fprintf(err_dhcp, "Username %s:\n", *user_name);
	fprintf(err_dhcp, "\tIP address : %s\n", (char*)inet_ntoa(addr.sin_addr));
	fprintf(err_dhcp, "\tIP subnet mask : %s\n", (char*)inet_ntoa(mask.sin_addr));
	fprintf(err_dhcp, "\tGateway address : %s\n", (char*)inet_ntoa(gateway.sin_addr));
	if (check_dns_name(lease))
        {	
            fprintf(err_dhcp, "\tDNS Server : %s\n", lease->dns); 
        }
        fprintf(err_dhcp, "\tDNS Server : %s\n", (char*)inet_ntoa(dns.sin_addr));
        fprintf(err_dhcp, "\tLease time : %ds\n", lease->lease_time);
        fprintf(err_dhcp, "\tRenew time : %ds\n", lease->renew_time);
        save_lease( lease, hw_addr, user_name );
    }
void save_lease( struct lease* lease, char * addr, char * username )
{
	char path[500] = {0};
	strcpy(path, DEFAULT_LEASE_PATH);
	int len = strlen(path);
	if (path[len - 1] != '/')
		path[len++] = '/';
	char cmd[600] = {0};
	sprintf(cmd, "mkdir -p %s\n", path);
	system(cmd);
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s%s.lease", path, username);
	fprintf(err_dhcp, "Saving lease to %s\n", cmd);
	FILE *fout = fopen(cmd, "w");
	if (!fout)
		return;
    struct interface * config_interface = malloc(sizeof(struct interface));
    memset(config_interface, 0, sizeof(struct interface));
    strcpy(config_interface->name, username);
	memcpy(config_interface->addr, addr, 6);
 
	fwrite(lease, 1, sizeof(struct lease), fout);
	fwrite(config_interface, 1, sizeof(struct interface), fout);
	fclose(fout);
}

int load_lease(struct lease* lease, char * addr, char * username)
{
	char path[500] = {0};
	strcpy(path, DEFAULT_LEASE_PATH);
	int len = strlen(path);
	if (path[len - 1] != '/')
		path[len++] = '/';
	char cmd[600] = {0};
	sprintf(cmd, "%s%s.lease", path, username);
	fprintf(err_dhcp, "Loading lease from %s\n", cmd);
	FILE *fin = fopen(cmd, "r");
	if (!fin)
		return 0;
	fread(lease, 1, sizeof(struct lease), fin);
	struct interface tmp_interface;
	fread(&(tmp_interface), 1, sizeof(struct interface), fin);
	fclose(fin);
    struct interface * config_interface = malloc(sizeof(struct interface));
    memset(config_interface, 0, sizeof(struct interface));
    strcpy(config_interface->name, username);
	memcpy(config_interface->addr, addr, 6);  

    if (memcmp(&tmp_interface, config_interface, sizeof(struct interface)) != 0)
		return 0;// lease does not match!
	if (portset) {//port set mode
		if (lease->portset_index == 0 && lease->portset_mask == 0)
			return 0; // invalid port set 
	} else {//no port set mode
		if (lease->portset_index != 0 || lease->portset_mask != 0)
			return 0; //invalid port set 
	}
	return 1;
}


