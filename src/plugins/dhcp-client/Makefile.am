#
#  OpenVPN (TM) DHCP Plugin -- PureVpn Plugin
#
#  Copyright (C) 2016      Dev Team <dev@purevpn.com>
#

MAINTAINERCLEANFILES = \
	                $(srcdir)/Makefile.in

AM_CFLAGS = -I$(top_srcdir)/include 

plugin_LTLIBRARIES = openvpn-plugin-dhcp-client.la
dist_doc_DATA = README.dhcp-client

openvpn_plugin_dhcp_client_la_SOURCES = dhcpclientdl.h dhcpclientdl.c \
				        interface.h interface.c \
					dhcp.h dhcp.c \
					socket.h socket.c \
					config-dhcp.h lease.h

openvpn_plugin_dhcp_client_la_LDFLAGS = $(AM_LDFLAGS) \
				        -module -shared -avoid-version -no-undefined
