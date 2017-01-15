/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dhcp-client.h
 * Author: Bond007
 *
 * Created on December 9, 2016, 6:17 PM
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "config-dhcp.h"
#include "dhcp.h"
#include "socket.h"
#include "interface.h";
#include "lease.h";

#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H
 
void dhcp_plugin_test( void );

void dhcp_client( char * username, char * dhcp_profile );

#endif /* DHCP_CLIENT_H */
