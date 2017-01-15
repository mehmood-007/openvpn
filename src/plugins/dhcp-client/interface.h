#ifndef __INTERFACE_H__
#define __INTERFACE_H__
#define _BSD_SOURCE
#define INTERFACE_NAME_LEN 100

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netdb.h>
#include "dhcp.h"
#include "config-dhcp.h"
#include "lease.h"

#define DEL 0
#define ADD 1

#define TCP 0
#define UDP 1
#define ICMP 2

struct interface {
	char name[INTERFACE_NAME_LEN];
	char addr[6];/* macaddr */
};

struct interface * network_interface;

void init_interfaces( char * if_name  );

void configure_interface( struct lease * lease,  char * addr, char * username );

void generate_mac( char * username, struct interface * config_interface);



#endif /* __INTERFACE_H__ */
