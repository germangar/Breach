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
// cl_serverlist.c  -- interactuates with the master server

#include "client.h"

//=========================================================

typedef struct serverlist_s
{
	char address[32];
	struct serverlist_s *pnext;
} serverlist_t;

serverlist_t *serverList;


static qboolean filter_allow_full = qfalse;
static qboolean filter_allow_empty = qfalse;

//=========================================================

//=================
//CL_FreeServerlist
//=================
static void CL_FreeServerlist( void )
{
	serverlist_t *ptr;

	while( serverList )
	{
		ptr = serverList;
		serverList = serverList->pnext;
		Mem_TempFree( ptr );
	}
}

//=================
//CL_ServerIsInList
//=================
static qboolean CL_ServerIsInList( char *adr )
{

	serverlist_t *server;

	if( !serverList )
		return qfalse;

	server = serverList;
	while( server )
	{
		if( !Q_stricmp( server->address, adr ) )
			return qtrue;
		server = server->pnext;
	}

	return qfalse;
}

//=================
//CL_AddServerToList
//=================
static qboolean CL_AddServerToList( char *adr )
{

	serverlist_t *newserv;
	netadr_t nadr;

	if( !adr || !strlen( adr ) )
		return qfalse;

	if( !NET_StringToAddress( adr, &nadr ) )
		return qfalse;

	if( CL_ServerIsInList( adr ) )
		return qfalse;

	newserv = (serverlist_t *)Mem_TempMalloc( sizeof( serverlist_t ) );
	Q_strncpyz( newserv->address, adr, sizeof( newserv->address ) );
	newserv->pnext = serverList;
	serverList = newserv;

	return qtrue;
}

//#define WRITESERVERLIST
#ifdef WRITESERVERLIST
//=================
//CL_LoadServerList
//=================
static qboolean CL_LoadServerList( void )
{

	qbyte *buf;
	char *ptr, *tok;
	int filehandle;
	int length;

	// load the file
	length = FS_FOpenFile( "serverlist.txt", &filehandle, FS_READ );
	if( length < 1 )
	{
		if( length == -1 )
			return qfalse;
		FS_FCloseFile( filehandle );
		return qfalse;
	}

	buf = Mem_TempMalloc( length + 1 );
	FS_Read( buf, length, filehandle );
	FS_FCloseFile( filehandle );
	if( !buf )
	{
		Mem_TempFree( buf );
		return qfalse;
	}

	// proceed
	ptr = ( char * )buf;
	while( ptr )
	{
		tok = COM_ParseExt( &ptr, qtrue );
		if( !tok[0] )
			break;

		CL_AddServerToList( tok );
	}


	Mem_TempFree( buf );
	return qtrue;
}

//=================
//CL_WriteServerList
//=================
static void CL_WriteServerList( void )
{

	int serversfile, length = 0;
	serverlist_t *server;

	if( !serverList )
		return;

	length = FS_FOpenFile( "serverlist.txt", &serversfile, FS_WRITE );
	if( length == -1 )
	{
		return;
	}

	server = serverList;
	while( server )
	{
		FS_Write( server->address, strlen( server->address ), serversfile );
		FS_Write( " ", 1, serversfile );
		server = server->pnext;
	}

	FS_FCloseFile( serversfile );
}
#endif

/*
* CL_ParseGetInfoResponse
* 
* Handle a server responding to a detailed info broadcast
*/
void CL_ParseGetInfoResponse( msg_t *msg )
{
	char *s = MSG_ReadString( msg );
	Com_DPrintf( "%s\n", s );
}

/*
* CL_ParseGetStatusResponse
* 
* Handle a server responding to a detailed info broadcast
*/
void CL_ParseGetStatusResponse( msg_t *msg )
{
	char *s = MSG_ReadString( msg );
	Com_DPrintf( "%s\n", s );
}

/*
* CL_QueryGetInfoMessage
*/
void CL_QueryGetInfoMessage( const char *cmdname )
{
	netadr_t adr;
	char *requeststring;
	char *server;

	//get what master
	server = Cmd_Argv( 1 );
	if( !server || !( *server ) )
	{
		Com_Printf( "no address provided %s...\n", server ? server : "" );
		return;
	}

	NET_Config( qtrue ); // allow remote

	requeststring = va( cmdname );

	// send a broadcast packet
	Com_DPrintf( "quering %s...\n", server );

	if( NET_StringToAddress( server, &adr ) )
	{
		if( !adr.port )
			adr.port = BigShort( PORT_SERVER );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", server );
	}
}

/*
* CL_QueryGetInfoMessage_f - getinfo ip:port
*/
void CL_QueryGetInfoMessage_f( void )
{
	CL_QueryGetInfoMessage( "getinfo" );
}


/*
* CL_QueryGetStatusMessage_f - getstatus ip:port
*/
void CL_QueryGetStatusMessage_f( void )
{
	CL_QueryGetInfoMessage( "getstatus" );
}

typedef struct
{
	unsigned int starttime;
	char address[MAX_QPATH];
} pingserver_t;
static pingserver_t pingServer;

void CL_PingServer_f( void )
{
	char *address_string;
	char requestString[32];
	netadr_t adr;

	if( Cmd_Argc() < 2 )
		Com_Printf( "Usage: pingserver [ip:port]\n" );

	address_string = Cmd_Argv( 1 );

	if( !Q_stricmp( address_string, pingServer.address ) && cls.realtime - pingServer.starttime < SERVER_PINGING_TIMEOUT )  // don't ping again before getting the reply
		return;

	Q_strncpyz( pingServer.address, address_string, sizeof( pingServer.address ) );
	pingServer.starttime = cls.realtime;

	Q_snprintfz( requestString, sizeof( requestString ), "info %i %s %s", APP_PROTOCOL_VERSION,
	             filter_allow_full ? "full" : "",
	             filter_allow_empty ? "empty" : "" );

	if( NET_StringToAddress( address_string, &adr ) )
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requestString );
}

/*
   =================
   CL_ParseStatusMessage

   Handle a reply from a ping
   =================
 */
void CL_ParseStatusMessage( msg_t *msg )
{
	char *s = MSG_ReadString( msg );

	Com_DPrintf( "%s\n", s );

	if( !Q_stricmp( NET_AddressToString( &net_from ), pingServer.address ) )
	{
		unsigned int ping = cls.realtime - pingServer.starttime;
		CL_UIModule_AddToServerList( NET_AddressToString( &net_from ), va( "\\\\ping\\\\%i%s", ping, s ) );
		pingServer.address[0] = 0;
		pingServer.starttime = 0;
		return;
	}

	CL_UIModule_AddToServerList( NET_AddressToString( &net_from ), s );
}

/*
   =================
   CL_ParseGetServersResponse

   Handle a reply from getservers message to master server
   =================
 */
static void CL_ParseGetServersResponse2( msg_t *msg )
{
	char adrString[32];
	qbyte addr[4];
	unsigned short port;
	netadr_t adr;

	MSG_BeginReading( msg );
	MSG_ReadLong( msg ); // skip the -1

	//jump over the command name
	if( !MSG_SkipData( msg, strlen( "getserversResponse\\" ) ) )
	{
		Com_Printf( "Invalid master packet ( missing getserversResponse )\n" );
		return;
	}

	while( msg->readcount +6 <= msg->cursize )
	{
		MSG_ReadData( msg, addr, 4 );
		port = BigShort( MSG_ReadShort( msg ) );
		if( 0 == port )
		{
			// last server seen
			break;
		}
		Q_snprintfz( adrString, sizeof( adrString ), "%u.%u.%u.%u:%u", addr[0], addr[1], addr[2], addr[3], port );

		Com_DPrintf( "%s\n", adrString );
		if( !NET_StringToAddress( adrString, &adr ) )
		{
			Com_Printf( "Bad address: %s\n", adrString );
			continue;
		}
		//Netchan_OutOfBandPrint( NS_CLIENT, adr, requestString );
		CL_AddServerToList( adrString );

		if( '\\' != MSG_ReadChar( msg ) )
		{
			Com_Printf( "Invalid master packet ( missing seperator )\n" );
			break;
		}
	}
}

//jal: I will remove this function soon
//jal: the browser bug was the response string being parsed by
//jal: using MSG_ParseStringLine instead of using MSG_ParseString.
//jal: MSG_ParseStringLine cutted off the string when it found
//jal: a line ending, a zero or a -1.
void CL_ParseGetServersResponse( msg_t *msg )
{
	CL_FreeServerlist();
	CL_ParseGetServersResponse2( msg );
#ifdef WRITESERVERLIST
	CL_LoadServerList(); //jal: tmp
	CL_WriteServerList();
#endif
#if 1
	//send the servers to the ui
	{
		serverlist_t *server;
		netadr_t adr;

		server = serverList;
		while( server )
		{
			if( NET_StringToAddress( server->address, &adr ) )
			{
				CL_UIModule_AddToServerList( server->address, "\\\\EOT" );
			}
			server = server->pnext;
		}
	}
#else
	{
		serverlist_t *server;
		char requestString[32];
		netadr_t adr;

		Q_snprintfz( requestString, sizeof( requestString ), "info %i %s %s", APP_PROTOCOL_VERSION,
		             filter_allow_full ? "full" : "",
		             filter_allow_empty ? "empty" : "" );

		server = serverList;
		while( server )
		{
			if( NET_StringToAddress( server->address, &adr ) )
				Netchan_OutOfBandPrint( NS_CLIENT, adr, requestString );
			server = server->pnext;
		}
	}
#endif
	CL_FreeServerlist();
}

/*
   =================
   CL_GetServers_f
   =================
 */
void CL_GetServers_f( void )
{
	netadr_t adr;
	char *requeststring;
	cvar_t *net_noudp;
	char gameName[MAX_QPATH];
	int i;
	char *modname;

	NET_Config( qtrue ); // allow remote

	net_noudp = Cvar_Get( "net_noudp", "0", CVAR_NOSET );

	filter_allow_full = qfalse;
	filter_allow_empty = qfalse;
	for( i = 0; i < Cmd_Argc(); i++ )
	{
		if( !Q_stricmp( "full", Cmd_Argv( i ) ) )
			filter_allow_full = qtrue;

		if( !Q_stricmp( "empty", Cmd_Argv( i ) ) )
			filter_allow_empty = qtrue;
	}

	if( !Q_stricmp( Cmd_Argv( 1 ), "local" ) )
	{
		// send a broadcast packet
		Com_Printf( "pinging broadcast...\n" );

		// erm... modname isn't sent in local queries?

		requeststring = va( "info %i %s %s", APP_PROTOCOL_VERSION,
		                    filter_allow_full ? "full" : "",
		                    filter_allow_empty ? "empty" : "" );

		if( !net_noudp->integer )
		{
			adr.type = NA_BROADCAST;
			adr.port = BigShort( PORT_SERVER );
			Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
		}
	}
	else if( !net_noudp->integer )
	{

		char *master;

		//get what master
		master = Cmd_Argv( 2 );
		if( !master || !( *master ) )
			return;

		modname = Cmd_Argv( 3 );
		if( !modname || !modname[0] || !Q_stricmp( modname, DEFAULT_BASEGAME ) )
		{                                                        // never allow anyone to use base dir as mod name
			Q_strncpyz( gameName, APPLICATION, sizeof( gameName ) );
		}
		else
		{
			Q_strncpyz( gameName, modname, sizeof( gameName ) );
		}
		gameName[0] = toupper( gameName[0] ); // make sure it starts with capital

		// create the message
		requeststring = va( "getservers %s %i %s %s", gameName, APP_PROTOCOL_VERSION,
		                    filter_allow_full ? "full" : "",
		                    filter_allow_empty ? "empty" : "" );

		// send a broadcast packet
		if( NET_StringToAddress( master, &adr ) )
		{
			adr.type = NA_IP;
			if( !adr.port )
				adr.port = BigShort( PORT_MASTER );
			Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );

			Com_Printf( "quering %s...%s\n", master, NET_AddressToString( &adr ) );
		}
		else
		{
			Com_Printf( "Bad address: %s\n", master );
		}
	}
}
