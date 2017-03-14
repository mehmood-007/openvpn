#include "dhcp.h"

/* generate random xid, while will be used throughout the client-server communication of DHCP*/
void 
generate_xid( uint32_t * xid )
{
  *xid = rand();
}
/* initiating dhcp-discover */
void 
init_dhcp(char * addr, char * username, char * dhcp_lease)
{
    struct dhcp_profile * dhcp_profile_t =  malloc( sizeof(struct dhcp_profile) );
    dhcp_profile_t->timeout_count = TIMEOUT_RETRY_TIMES; 
    dhcp_profile_t->listen_fd = init_socket();
    dhcp_profile_t->next_state = DISCOVER;
    dhcp_discover( addr, username, dhcp_lease, dhcp_profile_t );
    free(dhcp_profile_t);
}
/* preparing options, dhcp-param for creating dhcp-packet */
int 
gen_options( struct dhcp_packet * packet, char * username, 
             char * addr, STATE next_state, uint32_t client_ip, 
             uint32_t server_ip )
{
    int pos = 0;

    packet->options[pos++] = 0x63;
    packet->options[pos++] = 0x82;
    packet->options[pos++] = 0x53;
    packet->options[pos++] = 0x63;
    pos = gen_option_message_type(packet->options, pos, next_state);
    pos = gen_option_host_name(packet->options, pos, username);
    
   
    if( next_state != RELEASE)
    {
        pos = gen_option_parameter_request_list(packet->options, pos);
    }
    switch ( next_state )
    {
        case( REQUEST ):
            pos = gen_option_server_id( packet->options, pos, server_ip );
            pos = gen_option_ip_address( packet->options, pos, client_ip );           
            pos = gen_option_client_identifier( packet->options, pos, addr );
            break;
        case( RELEASE ):
            pos = gen_option_server_id( packet->options, pos, server_id );
            pos = gen_option_client_identifier( packet->options, pos, addr );
            break;
        case( DISCOVER ):
            pos = gen_option_client_identifier( packet->options, pos, addr );
            break;    
        default:
            break;
    }
    packet->options[pos++] = 0xFF;
    int len = sizeof( struct dhcp_packet ) - sizeof( packet->options ) + pos;
    return len;
}
/*set TLV for client-id in dhcp-packet */
int 
gen_option_client_identifier( uint8_t* options, int pos, char* addr )
{
    int len = 7;
    uint8_t htype = 1;
    options[pos++] = OPTION_CLIENTID;
    options[pos++] = len;
    options[pos++] = htype;
    memcpy( options + pos, addr, len );
    return pos + ( len - 1);    
}
/*setting dhcp-state (discover, request, release) */
int 
gen_option_message_type(uint8_t* options, int pos, STATE next_state)
{
    options[pos++] = OPTION_MESSAGETYPE;
    options[pos++] = 1;
    switch (next_state) 
    {
    case DISCOVER:
        options[pos++] = DISCOVER;
        break;
    case REQUEST:
        options[pos++] = REQUEST;
        break;
    case RELEASE:
        options[pos++] = RELEASE;
        break;
    default:
        fprintf(err_dhcp, "OVPN-DHCP: Unknown next_state!\n");
        return 0;
    }
    return pos;
}
/* set username or hostname for dhcp-request poacket */
int 
gen_option_host_name(uint8_t *options, int pos, char * username)
{
    int len = strlen( username );
    options[pos++] = OPTION_HOSTNAME;
    options[pos++] = len;
    memcpy(options + pos, username, len);
    return pos + len;
}
/* set dhcp-options list for requesting paramaetees in  dhcp-request packet */
int 
gen_option_parameter_request_list(uint8_t *options, int pos)
{
    options[pos++] = OPTION_PARAMETERREQUESTLIST;
    char *len = options + pos++;
    *len = 0;
    ++*len; options[pos++] = OPTION_SUBNETMASK;
    ++*len; options[pos++] = OPTION_BROADCAST;
    ++*len; options[pos++] = OPTION_ROUTER;
    ++*len; options[pos++] = OPTION_DOMAINNAME;
    ++*len; options[pos++] = OPTION_DNSSERVER;
    ++*len; options[pos++] = OPTION_SERVERID;
 
    return pos;
}
/* set dhcp-options server-ip-address for dhcp-request, dhcp-release */
int 
gen_option_server_id( uint8_t *options, int pos, uint32_t server_ip )
{
    options[pos++] = OPTION_SERVERID;
    options[pos++] = 4;
    *(uint32_t*)(options + pos) = server_ip;
    return pos + 4;
}
/* set dhcp-options client-ip-address for dhcp-request, dhcp-release */
int 
gen_option_ip_address( uint8_t * options, int pos, uint32_t client_ip )
{
    options[pos++] = OPTION_IPADDRESS;
    options[pos++] = 4;
    *(uint32_t*)(options + pos) = client_ip;
    return pos + 4;
}
/* responsible for creating DHCP packet */
static struct 
dhcp_packet* make_packet( int *len, uint32_t xid, STATE next_state, char * mac_addr, char * username, uint32_t client_ip, uint32_t server_ip )
{
    struct dhcp_packet *packet = malloc(sizeof(struct dhcp_packet));
    memset(packet, 0, sizeof(struct dhcp_packet));
    
    packet->op = BOOT_REQUEST;
    packet->htype = ETH_FLAG;
    packet->hlen = 6;
    packet->hops = 0;
    packet->xid = xid;
    packet->secs = 0;
    packet->flags = ntohs(BCAST_FLAG);
    packet->ciaddr = client_ip;
    packet->yiaddr = 0;
    packet->siaddr = 0;
    packet->giaddr = 0;
    memcpy( packet->chaddr, mac_addr, 6 );
    *len = gen_options ( packet, username, mac_addr, next_state, client_ip, server_ip);
    return packet;
}
/* sending DHCP-Discover packet to DHCP-Server */
void 
dhcp_discover( char * mac_addr, char * username, char * dhcp_lease, struct dhcp_profile * dhcp_profile_t )
{
    int len;
    if (dhcp_profile_t->next_state != DISCOVER) 
    {
        fprintf( err_dhcp, "OVPN-DHCP: State is not DHCP-DISCOVER!\n" );
        return;
    }
    generate_xid(&dhcp_profile_t->xid);
    dhcp_profile_t->renew = 0;

    struct dhcp_packet * packet = make_packet(&len, dhcp_profile_t->xid, dhcp_profile_t->next_state, mac_addr, username, 0, dhcp_profile_t->ack_lease.server_ip);
    send_packet( (char*)packet, len, dhcp_profile_t->listen_fd );
    free( packet );

    dhcp_profile_t->next_state = OFFER;
    dhcp_offer( mac_addr, username, dhcp_lease, dhcp_profile_t);
}
/* releasing ip which were assigned from dhcp-server */
void 
dhcp_release( char * mac_addr, char * username , uint32_t ip )
 {
    int len;
    uint32_t xid;
    uint32_t server_ip_ = ntohl(server_id);
    char s_id[16];
    generate_xid( &xid );
    struct dhcp_packet * packet = make_packet(&len, xid, RELEASE, mac_addr, username, ip, 0);
    
    sprintf( s_id, "%d.%d.%d.%d",
             (server_ip_ >> 24) & 0xFF,
             (server_ip_ >> 16) & 0xFF,
             (server_ip_ >>  8) & 0xFF,
             (server_ip_      ) & 0xFF );
   
    release_send_udp_ipv4( (char*)packet, len, s_id );
    free( packet );
 }
/* validated packet that if the packet meet the rules defined in RFC2131 */
int 
check_packet( struct dhcp_packet * packet, char * addr, uint32_t xid )
{
    if (packet->op != BOOT_REPLY) 
    {
        return 0;
    }
    if (packet->xid != xid) 
    {
        return 0;
    }
    if (memcmp(packet->chaddr, addr, 6) != 0) 
    {
        fprintf(stderr, "OVPN-DHCP: received packet mac address does not match!\n");
        return 0;
    }
    if (packet->options[0] != 0x63)
            return 0;
    if (packet->options[1] != 0x82)
            return 0;
    if (packet->options[2] != 0x53)
            return 0;
    if (packet->options[3] != 0x63)
            return 0;

    return 1;
}
/* Processing dhcp lease and assign parse dhcp-information according to RFC2131 */
void 
process_lease(struct lease* lease, struct dhcp_packet* packet)
{
    lease->client_ip = packet->yiaddr;
    uint8_t * p = packet->options + 4;
    uint8_t * dns_len ;
    while (PACKET_INSIDE(p, packet) && (*p & 0xff) != 0xff) 
    {
        switch (*p) 
        {
            case OPTION_DOMAINNAME:
                memcpy(lease->dns, p + 2, *(p + 1));
                break;
            case OPTION_DNSSERVER:               
                dns_len = p + 1;
                switch(*dns_len)
                {
                    case DNS_LENTH_SECONDARY:
                        lease->dns_ip_1 = *(uint32_t*)(p + 2);
                        lease->dns_ip_2 = *(uint32_t*)(p + 6);
                        break;
                    default:
                        lease->dns_ip_1 = *(uint32_t*)(p + 2);
                        lease->dns_ip_2 = 0;
                        break;
                }
                break;
            case OPTION_LEASETIME:
                break;
            case OPTION_RENEWALTIME:
                lease->renew_time = htonl(*(uint32_t*)(p + 2));
                break;
            case OPTION_ROUTER:
                lease->router_ip = *(uint32_t*)(p + 2);
                break;
            case OPTION_SERVERID:
                lease->server_ip = *(uint32_t*)(p + 2);
                break;
            case OPTION_SUBNETMASK:
                lease->mask_ip = *(uint32_t*)(p + 2);
                break;
            default:
                break;
        }
        p++;
        p += *p + 1;
    }
    if (lease->renew_time == 0)
        lease->renew_time = lease->lease_time / 2;
}
/* sending dhcp-offer to DHCP-Server */
void  
dhcp_offer( char * addr, char * username, char * dhcp_lease, struct dhcp_profile * dhcp_profile_t )
{
    /* As we haven't receive the offer from DHCP than, Bailout */
    if ( dhcp_profile_t->next_state != OFFER ) 
    {
        fprintf(err_dhcp, "State is not OFFER!\n");
        return;
    }
    struct dhcp_packet *packet = malloc(sizeof(struct dhcp_packet));
    memset(packet, 0, sizeof(struct dhcp_packet));
    int valid = 0;
    /* request dhcp-packet till we get the desired dhcp-packet */
    while ( !valid )
    {
        int len = recv_packet((char*)packet, sizeof(struct dhcp_packet), dhcp_profile_t->listen_fd );
        if (len < 0) 
        {
            if ( dhcp_profile_t->timeout_count-- ) /* timeout */
            {
                /* set dhcp-packet state to dhcp-discover and send to dhcp-server */
                dhcp_profile_t->next_state = DISCOVER;
                dhcp_discover( addr, username, dhcp_lease, dhcp_profile_t );
                return;
            }
            else 
            {
                fprintf(err_dhcp, "OVPN-DHCP: Error in getting dhcp_offer.......\n");
                free_socket( dhcp_profile_t->listen_fd );
                return;                
            }
        }
        /* vaildate dhcp-packet if a vaild is received proceed to further otherwise bailout */
        valid = check_packet(packet, addr, dhcp_profile_t->xid);
    }
    process_lease( &dhcp_profile_t->ack_lease, packet );	
    server_id = dhcp_profile_t->ack_lease.server_ip ;
    /* set retry count for DHCP-server */
    dhcp_profile_t->timeout_count = TIMEOUT_RETRY_TIMES;
    dhcp_profile_t->next_state = REQUEST;
    free( packet );
    /* send  */
    dhcp_request( addr, username, dhcp_lease, dhcp_profile_t);
}
void 
dhcp_request( char * mac_addr, char * username, char * dhcp_lease, struct dhcp_profile * dhcp_profile_t )
{
    int len;
    
    /* if not receive the desired response from the dhcp-packet */
    if (dhcp_profile_t->next_state != REQUEST) 
    {
        fprintf(err_dhcp, "OVPN-DHCP: State is not REQUEST!\n");
        free_socket( dhcp_profile_t->listen_fd );
        return;
    }
    /* create dhcp-packet from xid, dhcp-state, hw_addr, cid, sid */
    struct dhcp_packet * packet = make_packet( &len, dhcp_profile_t->xid, dhcp_profile_t->next_state , mac_addr, username, dhcp_profile_t->ack_lease.client_ip, dhcp_profile_t->ack_lease.server_ip);
    /* send created packet to dhcp-server */
    send_packet( (char*)packet, len, dhcp_profile_t->listen_fd );
    free(packet);

    dhcp_profile_t->next_state = ACK;
    dhcp_ack( mac_addr, username, dhcp_lease, dhcp_profile_t);
}
void 
dhcp_ack( char * addr, char * username, char * dhcp_lease_t, struct dhcp_profile * dhcp_profile_t )
{
    int valid = 0;
    int dhcp_err_ = 0;
    /* if not receive the desired response from the dhcp-packet */
    if (dhcp_profile_t->next_state != ACK) 
    {
        fprintf(err_dhcp, "OVPN-DHCP: State is not ACK!\n");
        return;
    }
    /* allocate memory for sending packet */
    struct dhcp_packet * packet = malloc(sizeof(struct dhcp_packet));
    memset(packet, 0, sizeof(struct dhcp_packet));
    while (!valid) 
    {
        int len = recv_packet((char*)packet, sizeof(struct dhcp_packet), dhcp_profile_t->listen_fd);
        if (len < 0) /* timeout */ 
        {
            if (dhcp_profile_t->timeout_count--)  /* Request DHCP-Server and wait for twice,*/ 
            {                                     /* if do not get response than bailout */         
                dhcp_profile_t->next_state = REQUEST; /* set next-state of DHCP to request  */
                dhcp_request( addr, username, dhcp_lease_t, dhcp_profile_t);
                return;    
            } 
            else 
            {
                fprintf(err_dhcp, "OVPN-DHCP: Error in DHCP-Ack.........\n");
                dhcp_err_ = 1;
                break;                                    
            }
        }
        /* vaildate dhcp-packet if a vaild is received proceed to further otherwise bailout */
        valid = check_packet(packet, addr, dhcp_profile_t->xid);
    }
    if( dhcp_err_ == 0 )
    {
        /* get leasing parameters received from the dhcp-server*/
        process_lease(&dhcp_profile_t->ack_lease, packet);        	
        /* save received the dhcp-parameters in struct, for main-openvpn fucntion ip-pool-acquire   */
        create_dhcp_profile( (struct dhcp_lease*)dhcp_lease_t, dhcp_profile_t->ack_lease, addr ); 
    }
    else 
    {
        /* create lease structure and set status to false as we haven't receive what we want */
       struct dhcp_lease * dhcp_lease_tt = (struct dhcp_lease*) dhcp_lease_t;
       dhcp_lease_tt->status = false;
    }
    /* releasing resources  */
    free( packet );
    free_socket( dhcp_profile_t->listen_fd );
}
/* storing the param recv from dhcp-server to pointer */
void
create_dhcp_profile( struct dhcp_lease * dhcp_profile, struct lease lease, char * addr )
{
    int mac_len = 6;
    dhcp_profile->dns_ip_1 = lease.dns_ip_1;
    dhcp_profile->dns_ip_2 = lease.dns_ip_2;
    dhcp_profile->mask_ip = lease.mask_ip ;
    dhcp_profile->client_ip = lease.client_ip;
    memcpy( dhcp_profile->addr, addr, mac_len );
    dhcp_profile->status = true;
}
