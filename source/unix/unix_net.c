/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_wins.c

#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <arpa/inet.h>

#ifdef NeXT
#include <libc.h>
#endif

netadr_t net_local_adr;

#define	LOOPBACK    0x7f000001

int ip_sockets[2];
int ipx_sockets[2];

static int NET_IPSocket( char *net_interface, int port );
char *NET_ErrorString( void );

//=============================================================================
//jal: perfect match with Q3
static void NetadrToSockadr( netadr_t *a, struct sockaddr_in *s )
{
	memset( s, 0, sizeof( *s ) );

	if( a->type == NA_BROADCAST )
	{
		s->sin_family = AF_INET;

		s->sin_port = a->port;
		*(int *)&s->sin_addr = -1;
	}
	else if( a->type == NA_IP )
	{
		s->sin_family = AF_INET;

		*(int *)&s->sin_addr = *(int *)&a->ip;
		s->sin_port = a->port;
	}
}

static void SockadrToNetadr( struct sockaddr_in *s, netadr_t *a )
{
	*(int *)&a->ip = *(int *)&s->sin_addr;
	a->port = s->sin_port;
	a->type = NA_IP;
}

static char *NET_BaseAdrToString( netadr_t *a )
{
	static char s[64];

	Q_snprintfz( s, sizeof( s ), "%i.%i.%i.%i", a->ip[0], a->ip[1], a->ip[2], a->ip[3] );

	return s;
}


/*
=============
Sys_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
static qboolean Sys_StringToSockaddr( const char *s, struct sockaddr *sadr )
{
	struct hostent *h;
	char *colon;
	char copy[128];

	memset( sadr, 0, sizeof( *sadr ) );
	( (struct sockaddr_in *)sadr )->sin_family = AF_INET;

	( (struct sockaddr_in *)sadr )->sin_port = 0;

	Q_strncpyz( copy, s, sizeof( copy ) );

	// strip off a trailing :port if present
	for( colon = copy; *colon; colon++ )
		if( *colon == ':' )
		{
			*colon = 0;
			( (struct sockaddr_in *)sadr )->sin_port = htons( (short)atoi( colon+1 ) );
		}

		if( copy[0] >= '0' && copy[0] <= '9' )
		{
			*(int *)&( (struct sockaddr_in *)sadr )->sin_addr = inet_addr( copy );
		}
		else
		{
			if( !( h = gethostbyname( copy ) ) )
				return qfalse;
			*(int *)&( (struct sockaddr_in *)sadr )->sin_addr = *(int *)h->h_addr_list[0];
		}

		return qtrue;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a )
{
	struct sockaddr_in sadr;

	if( !Sys_StringToSockaddr( s, (struct sockaddr *)&sadr ) )
		return qfalse;

	SockadrToNetadr( &sadr, a );

	return qtrue;
}

#define	MAX_IPS	    16
static int numIP;
static qbyte localIP[MAX_IPS][4];

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean    Sys_IsLANAddress( netadr_t adr )
{
	int i;

	if( adr.type == NA_LOOPBACK )
	{
		return qtrue;
	}

	if( adr.type != NA_IP )
	{
		return qfalse;
	}

	// ioQ3[start]
	// RFC1918:
	// 10.0.0.0        -   10.255.255.255  (10/8 prefix)
	// 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
	// 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
	if( adr.ip[0] == 10 )
		return qtrue;
	if( adr.ip[0] == 172 && ( adr.ip[1]&0xf0 ) == 16 )
		return qtrue;
	if( adr.ip[0] == 192 && adr.ip[1] == 168 )
		return qtrue;

	// the checks below are bogus, aren't they? -- ln
	// ioQ3[end]

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte

	// Class A
	if( ( adr.ip[0] & 0x80 ) == 0x00 )
	{
		for( i = 0; i < numIP; i++ )
		{
			if( adr.ip[0] == localIP[i][0] )
			{
				return qtrue;
			}
		}
		// the RFC1918 class a block will pass the above test
		return qfalse;
	}

	// Class B
	if( ( adr.ip[0] & 0xc0 ) == 0x80 )
	{
		for( i = 0; i < numIP; i++ )
		{
			if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] )
			{
				return qtrue;
			}
			// also check against the RFC1918 class b blocks
			if( adr.ip[0] == 172 && localIP[i][0] == 172 && ( adr.ip[1] & 0xf0 ) == 16 && ( localIP[i][1] & 0xf0 ) == 16 )
			{
				return qtrue;
			}
		}
		return qfalse;
	}

	// Class C
	for( i = 0; i < numIP; i++ )
	{
		if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2] )
		{
			return qtrue;
		}
		// also check against the RFC1918 class c blocks
		if( adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168 )
		{
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP( void )
{
	int i;

	for( i = 0; i < numIP; i++ )
	{
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
	}
}

/*
==================
NET_GetLocalAddress
==================
*/
static void NET_GetLocalAddress( void )
{
	char hostname[256];
	struct hostent *hostInfo;
	// int					error; // bk001204 - unused
	char *p;
	int ip;
	int n;

	if( gethostname( hostname, 256 ) == -1 )
	{
		return;
	}

	hostInfo = gethostbyname( hostname );
	if( !hostInfo )
	{
		return;
	}

	Com_Printf( "Hostname: %s\n", hostInfo->h_name );
	n = 0;
	while( ( p = hostInfo->h_aliases[n++] ) != NULL )
	{
		Com_Printf( "Alias: %s\n", p );
	}

	if( hostInfo->h_addrtype != AF_INET )
	{
		return;
	}

	numIP = 0;
	while( ( p = hostInfo->h_addr_list[numIP++] ) != NULL && numIP < MAX_IPS )
	{
		ip = ntohl( *(int *)p );
		localIP[numIP][0] = p[0];
		localIP[numIP][1] = p[1];
		localIP[numIP][2] = p[2];
		localIP[numIP][3] = p[3];
		Com_Printf( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );
	}
}

//=============================================================================

qboolean Sys_GetPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message )
{
	int ret;
	struct sockaddr_in from;
	socklen_t fromlen;
	int net_socket;
	int protocol;
	int err;

	for( protocol = 0; protocol < 2; protocol++ )
	{
		if( protocol == 0 )
			net_socket = ip_sockets[sock];
		else
			net_socket = ipx_sockets[sock];

		if( !net_socket )
			continue;

		fromlen = sizeof( from );
		ret = recvfrom( net_socket, net_message->data, net_message->maxsize
			, 0, (struct sockaddr *)&from, &fromlen );

		SockadrToNetadr( &from, net_from );
		// bk000305: was missing
		net_message->readcount = 0; // jal : taken from Q3

		if( ret == -1 )
		{
			err = errno;

			if( err == EWOULDBLOCK || err == ECONNREFUSED )
				continue;
			Com_Printf( "NET_GetPacket: %s from %s\n", NET_ErrorString(),
				NET_AdrToString( net_from ) );
			continue;
		}

		if( ret == net_message->maxsize )
		{
			Com_Printf( "Oversize packet from %s\n", NET_AdrToString( net_from ) );
			continue;
		}

		net_message->cursize = ret;
		return qtrue;
	}

	return qfalse;
}

//=============================================================================

/*
=====================
Sys_SendPacket

System specific. Loopback connections have already been filtered out at NET_SendPacket
=====================
*/
void Sys_SendPacket( netsrc_t sock, size_t length, void *data, netadr_t to )
{
	int ret;
	struct sockaddr_in addr;
	int net_socket;

	if( to.type == NA_BROADCAST )
	{
		net_socket = ip_sockets[sock];
		if( !net_socket )
			return;
	}
	else if( to.type == NA_IP )
	{
		net_socket = ip_sockets[sock];
		if( !net_socket )
			return;
	}
	else
	{
		Com_Error( ERR_FATAL, "NET_SendPacket: bad address type" );
		return;
	}

	NetadrToSockadr( &to, &addr );

	ret = sendto( net_socket, data, length, 0, (struct sockaddr *)&addr, sizeof( addr ) );
	if( ret == -1 )
	{
		Com_Printf( "NET_SendPacket ERROR: %s to %s\n", NET_ErrorString(),
			NET_AdrToString( &to ) );
	}
}

//=============================================================================

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP( void )
{
	cvar_t *port, *ip;

	NET_GetLocalAddress(); // wsw : jal : copy/paste from Q3

	port = Cvar_Get( "port", va( "%i", PORT_SERVER ), CVAR_NOSET );
	ip = Cvar_Get( "ip", "localhost", CVAR_NOSET );

	if( !ip_sockets[NS_SERVER] )
		ip_sockets[NS_SERVER] = NET_IPSocket( ip->string, port->integer );

	if( dedicated->integer )
	{
		if( !ip_sockets[NS_SERVER] )
			Com_Error( ERR_FATAL, "Couldn't allocate dedicated server IP port" );
		return;
	}

	if( !ip_sockets[NS_CLIENT] )
		ip_sockets[NS_CLIENT] = NET_IPSocket( ip->string, PORT_ANY );
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void	NET_Config( qboolean multiplayer )
{
	int i;
	static qboolean	old_config;

	if( old_config == multiplayer )
		return;

	old_config = multiplayer;

	if( !multiplayer )
	{ // shut down any existing sockets
		for( i = 0; i < 2; i++ )
		{
			if( ip_sockets[i] )
			{
				close( ip_sockets[i] );
				ip_sockets[i] = 0;
			}
			if( ipx_sockets[i] )
			{
				close( ipx_sockets[i] );
				ipx_sockets[i] = 0;
			}
		}
	}
	else
	{ // open sockets
		NET_OpenIP();
	}
}

//===================================================================

/*
====================
NET_Init
====================
*/
void NET_Init( void )
{
}


/*
====================
NET_IPSocket
====================
*/
static int NET_IPSocket( char *net_interface, int port )
{
	int newsocket;
	struct sockaddr_in address;
	int _true = 1;
	int i = 1;

	if( net_interface )
	{
		Com_Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	}
	else
	{
		Com_Printf( "Opening IP socket: localhost:%i\n", port );
	}

	if( ( newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == -1 )
	{
		Com_Printf( "ERROR: UDP_OpenSocket: socket: %s", NET_ErrorString() );
		return 0;
	}

	// make it non-blocking
	if( ioctl( newsocket, FIONBIO, &_true ) == -1 )
	{
		Com_Printf( "ERROR: UDP_OpenSocket: ioctl FIONBIO:%s\n", NET_ErrorString() );
		return 0;
	}

	// make it broadcast capable
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof( i ) ) == -1 )
	{
		Com_Printf( "ERROR: UDP_OpenSocket: setsockopt SO_BROADCAST:%s\n", NET_ErrorString() );
		return 0;
	}

	if( !net_interface || !net_interface[0] || !strcmp( net_interface, "localhost" ) )
	{
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		if( !Sys_StringToSockaddr( net_interface, (struct sockaddr *)&address ) )
		{
			Com_Printf( "ERROR: UDP_OpenSocket: Couldn't resolve interface address: %s\n", net_interface );
			return 0;
		}
	}

	if( port == PORT_ANY )
		address.sin_port = 0;
	else
		address.sin_port = htons( (short)port );

	address.sin_family = AF_INET;

	if( bind( newsocket, (void *)&address, sizeof( address ) ) == -1 )
	{
		Com_Printf( "ERROR: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
		close( newsocket );
		return 0;
	}

	return newsocket;
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown( void )
{
	NET_Config( qfalse ); // close sockets
}


/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void )
{
	int code;

	code = errno;
	return strerror( code );
}

// sleeps msec or until net socket is ready
void NET_Sleep( int msec )
{
	struct timeval timeout;
	fd_set fdset;
	extern cvar_t *dedicated;
	extern qboolean stdin_active;

	if( !ip_sockets[NS_SERVER] || ( dedicated && !dedicated->integer ) )
		return; // we're not a server, just run full speed

	FD_ZERO( &fdset );
	if( stdin_active )
		FD_SET( 0, &fdset ); // stdin is processed too
	FD_SET( ip_sockets[NS_SERVER], &fdset ); // network socket
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = ( msec%1000 )*1000;
	select( ip_sockets[NS_SERVER]+1, &fdset, NULL, NULL, &timeout );
}
