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

#include "winsock.h"
#include "../qcommon/qcommon.h"


cvar_t *net_shownet;


static cvar_t *net_noudp;

static WSADATA winsockdata;

int ip_sockets[2];


//=============================================================================

/*
* NET_ErrorString
*/
char *NET_ErrorString( void )
{
	int code;

	code = WSAGetLastError();
	switch( code )
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}

static void NetadrToSockadr( netadr_t *a, struct sockaddr *s )
{
	memset( s, 0, sizeof( *s ) );

	if( a->type == NA_BROADCAST )
	{
		( (struct sockaddr_in *)s )->sin_family = AF_INET;
		( (struct sockaddr_in *)s )->sin_port = a->port;
		( (struct sockaddr_in *)s )->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if( a->type == NA_IP )
	{
		( (struct sockaddr_in *)s )->sin_family = AF_INET;
		( (struct sockaddr_in *)s )->sin_addr.s_addr = *(int *)&a->ip;
		( (struct sockaddr_in *)s )->sin_port = a->port;
	}
}

static void SockadrToNetadr( struct sockaddr *s, netadr_t *a )
{
	if( s->sa_family == AF_INET )
	{
		a->type = NA_IP;
		*(int *)&a->ip = ( (struct sockaddr_in *)s )->sin_addr.s_addr;
		a->port = ( (struct sockaddr_in *)s )->sin_port;
	}
}

/*
* Sys_StringToSockaddr
*
* localhost
* idnewt
* idnewt:28000
* 192.246.40.70
* 192.246.40.70:28000
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
* Sys_StringToAdr
*
* localhost
* idnewt
* idnewt:28000
* 192.246.40.70
* 192.246.40.70:28000
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a )
{
	struct sockaddr sadr;

	if( !Sys_StringToSockaddr( s, &sadr ) )
	{
		return qfalse;
	}

	SockadrToNetadr( &sadr, a );
	return qtrue;
}

/*
* Sys_GetPacket
*
* System specific. Loopback connections have already been filtered out at NET_GetPacket
*/
qboolean Sys_GetPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message )
{
	int ret;
	struct sockaddr from;
	int fromlen;
	int net_socket;
	int protocol;
	int err;

	for( protocol = 0; protocol < 1; protocol++ )
	{
		net_socket = ip_sockets[sock];
		if( !net_socket )
			continue;

		fromlen = sizeof( from );

		ret = recvfrom( net_socket, (char *)net_message->data, net_message->maxsize, 0, (struct sockaddr *)&from, &fromlen );
		if( ret == SOCKET_ERROR )
		{
			err = WSAGetLastError();
			if( err == WSAEWOULDBLOCK || err == WSAECONNRESET )
			{
				continue;
			}
			if( err == WSAEMSGSIZE )
			{
				Com_Printf( "Warning:  Oversize packet from %s\n", NET_AddressToString( net_from ) );
				continue;
			}

			// let servers continue after errors
			Com_DPrintf( "NET_GetPacket: %s from %s\n", NET_ErrorString(), NET_AddressToString( net_from ) );
			continue;
		}

		SockadrToNetadr( &from, net_from );

		if( ret == (int) net_message->maxsize )
		{
			Com_Printf( "Oversize packet from %s\n", NET_AddressToString( net_from ) );
			continue;
		}

		net_message->cursize = ret;
		return qtrue;
	}

	return qfalse;
}

/*
* Sys_SendPacket
*
* System specific. Loopback connections have already been filtered out at NET_SendPacket
*/
void Sys_SendPacket( netsrc_t sock, size_t length, void *data, netadr_t to )
{
	int ret;
	struct sockaddr	addr;
	SOCKET net_socket;

	if( to.type == NA_BROADCAST )
	{
		net_socket = ip_sockets[sock];
	}
	else if( to.type == NA_IP )
	{
		net_socket = ip_sockets[sock];
	}
	else
	{
		Com_Error( ERR_FATAL, "Sys_SendPacket: bad address type" );
		return;
	}

	if( !net_socket )
	{
		return;
	}

	NetadrToSockadr( &to, &addr );

	ret = sendto( net_socket, data, length, 0, &addr, sizeof( addr ) );

	if( ret == SOCKET_ERROR )
	{
		int err = WSAGetLastError();

		// wouldblock is silent
		if( err == WSAEWOULDBLOCK || WSAECONNRESET )
		{                                      // WSAECONNRESET taken from r1q2
			return;
		}

		// some PPP links do not allow broadcasts and return an error
		if( ( err == WSAEADDRNOTAVAIL ) && ( to.type == NA_BROADCAST ) )
		{
			return;
		}

		if( dedicated->integer )
		{                   // let dedicated servers continue after errors
			Com_Printf( "NET_SendPacket ERROR: %s to %s\n",
				NET_ErrorString(), NET_AddressToString( &to ) );
		}
		else
		{
			if( err == WSAEADDRNOTAVAIL )
			{
				Com_DPrintf( "NET_SendPacket Warning: %s : %s\n",
					NET_ErrorString(), NET_AddressToString( &to ) );
			}
			else
			{
				Com_Error( ERR_DROP, "NET_SendPacket ERROR: %s to %s",
					NET_ErrorString(), NET_AddressToString( &to ) );
			}
		}
	}
}

#define	MAX_IPS	    16
static int numIP;
static qbyte localIP[MAX_IPS][4];

/*
* Sys_IsLANAddress
*
* LAN clients will have their rate var ignored
*/
qboolean Sys_IsLANAddress( netadr_t adr )
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

	if( adr.ip[0] == 127 && adr.ip[1] == 0 && adr.ip[2] == 0 && adr.ip[3] == 1 )
	{
		return qtrue;
	}

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
* Sys_ShowIP
*/
void Sys_ShowIP( void )
{
	int i;

	for( i = 0; i < numIP; i++ )
	{
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
	}
}

//=============================================================================


/*
* NET_IPSocket
*/
static int NET_IPSocket( char *net_interface, int port )
{
	SOCKET newsocket;
	struct sockaddr_in address;
	unsigned long _true = 1;
	int i = 1;
	int err;

	if( net_interface )
	{
		Com_Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	}
	else
	{
		Com_Printf( "Opening IP socket: localhost:%i\n", port );
	}

	if( ( newsocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET )
	{
		err = WSAGetLastError();
		if( err != WSAEAFNOSUPPORT )
		{
			Com_Printf( "WARNING: UDP_OpenSocket: socket: %s\n", NET_ErrorString() );
		}
		return 0;
	}

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR )
	{
		Com_Printf( "WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		return 0;
	}

	// make it broadcast capable
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof( i ) ) == SOCKET_ERROR )
	{
		Com_Printf( "WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
		return 0;
	}

	if( !net_interface || !net_interface[0] || !Q_stricmp( net_interface, "localhost" ) )
	{
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		if( !Sys_StringToSockaddr( net_interface, (struct sockaddr *)&address ) )
		{
			Com_Printf( "WARNING: UDP_OpenSocket: Couldn't resolve interface address: %s\n", net_interface );
			return 0;
		}
	}

	if( port == PORT_ANY )
	{
		address.sin_port = 0;
	}
	else
	{
		address.sin_port = htons( (short)port );
	}

	address.sin_family = AF_INET;

	if( bind( newsocket, (void *)&address, sizeof( address ) ) == SOCKET_ERROR )
	{
		Com_Printf( "WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	return newsocket;
}

/*
* NET_GetLocalAddress
*/
static void NET_GetLocalAddress( void )
{
	char hostname[256];
	struct hostent *hostInfo;
	int error;
	char *p;
	int ip;
	int n;

	if( gethostname( hostname, 256 ) == SOCKET_ERROR )
	{
		error = WSAGetLastError();
		return;
	}

	hostInfo = gethostbyname( hostname );
	if( !hostInfo )
	{
		error = WSAGetLastError();
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
	while( ( p = hostInfo->h_addr_list[numIP] ) != NULL && numIP < MAX_IPS )
	{
		ip = ntohl( *(int *)p );
		localIP[numIP][0] = p[0];
		localIP[numIP][1] = p[1];
		localIP[numIP][2] = p[2];
		localIP[numIP][3] = p[3];
		Com_Printf( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );
		numIP++;
	}
}

/*
* NET_OpenIP
*/
static void NET_OpenIP( void )
{
	cvar_t *ip, *port;

	ip = Cvar_Get( "ip", "localhost", CVAR_LATCH );
	port = Cvar_Get( "port", va( "%i", PORT_SERVER ), CVAR_LATCH );

	if( !ip_sockets[NS_SERVER] )
	{
		// try user ip:port
		ip_sockets[NS_SERVER] = NET_IPSocket( ip->string, port->integer );
		if( !ip_sockets[NS_SERVER] )
		{
			// try local ip
			NET_GetLocalAddress();
		}
	}

	if( dedicated->integer )
	{
		if( !ip_sockets[NS_SERVER] )
		{
			Com_Error( ERR_FATAL, "Couldn't allocate dedicated server IP port" );
		}
		return;
	}

	if( !ip_sockets[NS_CLIENT] )
	{
		srand( Sys_Milliseconds() );
		port = Cvar_Get( "clientport", va( "%i", PORT_CLIENT ), CVAR_NOSET );

		if( port->integer )
			ip_sockets[NS_CLIENT] = NET_IPSocket( ip->string, port->integer );

		if( !ip_sockets[NS_CLIENT] )
			ip_sockets[NS_CLIENT] = NET_IPSocket( ip->string, PORT_ANY );
	}
}

// sleeps msec or until net socket is ready
void NET_Sleep( int msec )
{
	struct timeval timeout;
	fd_set fdset;
	extern cvar_t *dedicated;
	int i;

	if( !dedicated || !dedicated->integer )
		return; // we're not a server, just run full speed

	FD_ZERO( &fdset );
	i = 0;
	if( ip_sockets[NS_SERVER] )
	{
		FD_SET( ip_sockets[NS_SERVER], &fdset ); // network socket
		i = ip_sockets[NS_SERVER];
	}
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = ( msec%1000 )*1000;
	select( i+1, &fdset, NULL, NULL, &timeout );
}

//===================================================================

/*
* NET_GetCvars
*/
static qboolean NET_GetCvars( void )
{
	net_noudp = Cvar_Get( "net_noudp", "0", CVAR_NOSET );
	net_shownet = Cvar_Get( "net_shownet", "0", 0 );
	return qtrue;
}

/*
* NET_Config
*
* A single player game will only use the loopback code
*/
void NET_Config( qboolean multiplayer )
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
				closesocket( ip_sockets[i] );
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{ // open sockets
		if( !net_noudp->integer )
			NET_OpenIP();
	}
}

/*
* NET_Init
*/
void NET_Init( void )
{
	int r;

	r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r )
		Com_Error( ERR_FATAL, "Winsock initialization failed." );

	Com_Printf( "Winsock Initialized\n" );

	// this is really just to get the cvars registered
	NET_GetCvars();
}


/*
* NET_Shutdown
*/
void NET_Shutdown( void )
{
	NET_Config( qfalse ); // close sockets
	WSACleanup();
}
