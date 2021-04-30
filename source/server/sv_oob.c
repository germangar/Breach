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

#include "server.h"

static netadr_t master_adr[MAX_MASTERS];    // address of group servers

extern cvar_t *sv_masterservers;
extern cvar_t *sv_hostname;
extern cvar_t *sv_skilllevel;
extern cvar_t *sv_reconnectlimit;     // minimum seconds between connect messages
extern cvar_t *rcon_password;         // password for remote server commands


//==============================================================================
//
//MASTER SERVERS MANAGEMENT
//
//==============================================================================


//====================
//SV_AddMaster_f
//Add a master server to the list
//====================
static void SV_AddMaster_f( char *master )
{
	int i;

	if( !master || !master[0] )
		return;

	if( !sv_public->integer )
	{
		Com_Printf( "'SV_AddMaster_f' Only public servers use masters.\n" );
		return;
	}

	//never go public when not acting as a game server
	if( sv.state > ss_game )
		return;

	for( i = 0; i < MAX_MASTERS; i++ )
	{
		if( master_adr[i].port )
			continue;

		if( !NET_StringToAddress( master, &master_adr[i] ) )
		{
			Com_Printf( "'SV_AddMaster_f' Bad Master server address: %s\n", master );
			return;
		}
		if( master_adr[i].port == 0 )
			master_adr[i].port = BigShort( PORT_MASTER );

		Com_Printf( "Added new master server #%i at %s\n", i, NET_AddressToString( &master_adr[i] ) );
		return;
	}

	Com_Printf( "'SV_AddMaster_f' List of master servers is already full\n" );
}

//====================
//SV_InitMaster
//Set up the main master server
//====================
void SV_InitMaster( void )
{
	int i;
	char *master, *mlist;

	//  initialize masters list
	for( i = 0; i < MAX_MASTERS; i++ )
		memset( &master_adr[i], 0, sizeof( master_adr[i] ) );

	// never go public when not acting as a game server
	if( sv.state > ss_game )
		return;

	if( !sv_public->integer )
		return;

	mlist = sv_masterservers->string;
	if( *mlist )
	{
		while( mlist )
		{
			master = COM_Parse( &mlist );
			if( !master[0] )
				break;

			SV_AddMaster_f( master );
		}
	}

	svc.last_heartbeat = HEARTBEAT_SECONDS * 1000; // wait a while before sending first heartbeat
}

//================
//SV_MasterHeartbeat
//Send a message to the master every few minutes to
//let it know we are alive, and log information
//================
void SV_MasterHeartbeat( void )
{
	int i;

	svc.last_heartbeat -= svc.snapFrameTime;
	if( svc.last_heartbeat > 0 )
		return;

	svc.last_heartbeat = HEARTBEAT_SECONDS * 1000;

	if( !sv_public->integer )
		return;

	// never go public when not acting as a game server
	if( sv.state > ss_game )
		return;

	// send to group master
	for( i = 0; i < MAX_MASTERS; i++ )
	{
		if( master_adr[i].port )
		{
			if( dedicated && dedicated->integer )
				Com_Printf( "Sending heartbeat to %s\n", NET_AddressToString( &master_adr[i] ) );
			Netchan_OutOfBandPrint( NS_SERVER, master_adr[i], "heartbeat %s\n", "DarkPlaces" );
		}
	}
}

//============================================================================

//===============
//SV_LongInfoString
//Builds the string that is sent as heartbeats and status replies
//===============
static char *SV_LongInfoString( qboolean fullStatus )
{
	char tempstr[1024] = { 0 };
	const char *gametype;
	static char status[MAX_MSGLEN - 16];
	int i, bots, count;
	client_t *cl;
	size_t statusLength;
	size_t tempstrLength;

	// convert "g_gametype" to "gametype". Ugly, but a must do because master servers asume g_gametype *must* be an integer
	gametype = Info_ValueForKey( status, "g_gametype" );
	if( gametype )
	{
		Info_RemoveKey( status, "g_gametype" );
		Info_SetValueForKey( status, "gametype", gametype );
	}

	Q_strncpyz( status, Cvar_Serverinfo(), sizeof( status ) );
	statusLength = strlen( status );

	bots = 0;
	count = 0;
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		cl = &svs.clients[i];
		if( cl->state >= CS_CONNECTED )
		{
			if( cl->fakeClient )
				bots++;
			count++;
		}
	}

	if( bots )
		Q_snprintfz( tempstr, sizeof( tempstr ), "\\bots\\%i", bots );
	Q_snprintfz( tempstr + strlen( tempstr ), sizeof( tempstr ) - strlen( tempstr ), "\\clients\\%i%s", count, fullStatus ? "\n" : "" );
	tempstrLength = strlen( tempstr );
	if( statusLength + tempstrLength >= sizeof( status ) )
		return status; // can't hold any more
	Q_strncpyz( status + statusLength, tempstr, sizeof( status ) - statusLength );
	statusLength += tempstrLength;

	if ( fullStatus )
	{
		for( i = 0; i < sv_maxclients->integer; i++ )
		{
			cl = &svs.clients[i];
			if( cl->state >= CS_CONNECTED )
			{
				Q_snprintfz( tempstr, sizeof( tempstr ), "\"%s\"\n", cl->name );
				tempstrLength = strlen( tempstr );
				if( statusLength + tempstrLength >= sizeof( status ) )
					break; // can't hold any more
				Q_strncpyz( status + statusLength, tempstr, sizeof( status ) - statusLength );
				statusLength += tempstrLength;
			}
		}
	}

	return status;
}

//================
//SV_ShortInfoString
//Generates a short info string for broadcast scan replies
//================
#define MAX_STRING_SVCINFOSTRING 160
#define MAX_SVCINFOSTRING_LEN ( MAX_STRING_SVCINFOSTRING - 4 )
static char *SV_ShortInfoString( void )
{
	static char string[MAX_STRING_SVCINFOSTRING];
	char hostname[64];
	char entry[16];
	size_t len;
	int i, count, bots;

	bots = 0;
	count = 0;
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state >= CS_CONNECTED )
		{
			if( svs.clients[i].fakeClient )
				bots++;
			count++;
		}
	}

	//format:
	//" \377\377\377\377info\\n\\server_name\\m\\map name\\u\\clients/maxclients\\g\\gametype\\s\\skill\\EOT "

	if( sv_skilllevel->integer > 2 )
		Cvar_ForceSet( "sv_skilllevel", "2" );
	if( sv_skilllevel->integer < 0 )
		Cvar_ForceSet( "sv_skilllevel", "0" );

	Q_strncpyz( hostname, sv_hostname->string, sizeof( hostname ) );
	Q_snprintfz( string, sizeof( string ),
	             "\\\\n\\\\%s\\\\m\\\\%8s\\\\u\\\\%2i/%2i\\\\",
	             hostname,
	             sv.mapname,
	             count > 99 ? 99 : count,
	             sv_maxclients->integer > 99 ? 99 : sv_maxclients->integer
	);

	len = strlen( string );
	Q_snprintfz( entry, sizeof( entry ), "g\\\\%5s\\\\", Cvar_String( "g_gametype" ) );
	if( MAX_SVCINFOSTRING_LEN - len > strlen( entry ) )
	{
		Q_strncatz( string, entry, sizeof( string ) );
		len = strlen( string );
	}

	Q_snprintfz( entry, sizeof( entry ), "s\\\\%1d\\\\", sv_skilllevel->integer );
	if( MAX_SVCINFOSTRING_LEN - len > strlen( entry ) )
	{
		Q_strncatz( string, entry, sizeof( string ) );
		len = strlen( string );
	}

	if( ( strlen( Cvar_String( "password" ) ) > 0 ) )
	{
		Q_snprintfz( entry, sizeof( entry ), "p\\\\%i\\\\", ( strlen( Cvar_String( "password" ) ) > 0 ) );
		if( MAX_SVCINFOSTRING_LEN - len > strlen( entry ) )
		{
			Q_strncatz( string, entry, sizeof( string ) );
			len = strlen( string );
		}
	}

	if( bots )
	{
		Q_snprintfz( entry, sizeof( entry ), "b\\\\%2i\\\\", bots > 99 ? 99 : bots );
		if( MAX_SVCINFOSTRING_LEN - len > strlen( entry ) )
		{
			Q_strncatz( string, entry, sizeof( string ) );
			len = strlen( string );
		}
	}

	// finish it
	Q_strncatz( string, "EOT", sizeof( string ) );
	return string;
}



//==============================================================================
//
//OUT OF BAND COMMANDS
//
//==============================================================================


//================
//SVC_Ack
//================
static void SVC_Ack( void )
{
	Com_Printf( "Ping acknowledge from %s\n", NET_AddressToString( &net_from ) );
}

//================
//SVC_Ping
//Just responds with an acknowledgement
//================
static void SVC_Ping( void )
{
	Netchan_OutOfBandPrint( NS_SERVER, net_from, "ack" );
}

//================
//SVC_InfoResponse
//
//Responds with short info for broadcast scans
//The second parameter should be the current protocol version number.
//================
static void SVC_InfoResponse( void )
{
	int i, count;
	char *string;
	qboolean allow_empty = qfalse, allow_full = qfalse;

	// ignore when private
	if( !sv_public->integer || sv_maxclients->integer == 1 )
		return;

	// ignore when in invalid server state
	if( sv.state < ss_loading || sv.state > ss_game )
		return;

	// different protocol version
	if( atoi( Cmd_Argv( 1 ) ) != APP_PROTOCOL_VERSION )
		return;

	// check for full/empty filtered states
	for( i = 0; i < Cmd_Argc(); i++ )
	{
		if( !Q_stricmp( Cmd_Argv( i ), "full" ) )
			allow_full = qtrue;

		if( !Q_stricmp( Cmd_Argv( i ), "empty" ) )
			allow_empty = qtrue;
	}

	count = 0;
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state >= CS_CONNECTED )
		{
			count++;
		}
	}

	if( ( count == sv_maxclients->integer ) && !allow_full )
	{
		return;
	}

	if( ( count == 0 ) && !allow_empty )
	{
		return;
	}

	string = SV_ShortInfoString();
	if( string )
		Netchan_OutOfBandPrint( NS_SERVER, net_from, "info\n%s", string );
}

//================
//SVC_SendInfoString
//================
static void SVC_SendInfoString( const char *requestType, const char *responseType, qboolean fullStatus )
{
	char *string;

	// ignore when private
	if( !sv_public->integer || sv_maxclients->integer == 1 )
		return;

	// ignore when in invalid server state
	if( sv.state < ss_loading || sv.state > ss_game )
		return;

	// send the same string that we would give for a status OOB command
	string = SV_LongInfoString( fullStatus );
	if( string )
		Netchan_OutOfBandPrint( NS_SERVER, net_from, "%s\n\\challenge\\%s%s", responseType, Cmd_Argv( 1 ), string );

}

//================
//SVC_GetInfoResponse
//================
static void SVC_GetInfoResponse( void )
{
	SVC_SendInfoString( "GetInfo", "infoResponse", qfalse );
}

//================
//SVC_GetStatusResponse
//================
static void SVC_GetStatusResponse( void )
{
	SVC_SendInfoString( "GetStatus", "statusResponse", qtrue );
}

//=================
//SVC_GetChallenge
//
//Returns a challenge number that can be used
//in a subsequent client_connect command.
//We do this to prevent denial of service attacks that
//flood the server with invalid connection IPs.  With a
//challenge, they must give a valid IP address.
//=================
static void SVC_GetChallenge( void )
{
	int i;
	int oldest;
	int oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for( i = 0; i < MAX_CHALLENGES; i++ )
	{
		if( NET_CompareBaseAdr( &net_from, &svs.challenges[i].adr ) )
			break;
		if( svs.challenges[i].time < oldestTime )
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if( i == MAX_CHALLENGES )
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = Sys_Milliseconds();
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint( NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge );
}


//==================
//SVC_DirectConnect
//A connection request that did not come from the master
//==================
static void SVC_DirectConnect( void )
{
	int i;
	char userinfo[MAX_INFO_STRING];
	netadr_t adr;
	client_t *cl, *newcl;
	int version;
	int qport;
	int challenge;

	adr = net_from;

	Com_DPrintf( "SVC_DirectConnect ()\n" );

	version = atoi( Cmd_Argv( 1 ) );
	if( version != APP_PROTOCOL_VERSION )
	{
		if( version <= 6 )
		{            // before reject packet was added
			Netchan_OutOfBandPrint( NS_SERVER, adr, "print\nServer is version %4.2f. Protocol %3i\n",
			                        APP_VERSION, APP_PROTOCOL_VERSION );
		}
		else
		{
			Netchan_OutOfBandPrint( NS_SERVER, adr, "reject\n%i\n%i\nServer and client don't have the same version\n",
			                        DROP_TYPE_GENERAL, 0 );
		}
		Com_DPrintf( "    rejected connect from protocol %i\n", version );
		return;
	}

	qport = atoi( Cmd_Argv( 2 ) );
	challenge = atoi( Cmd_Argv( 3 ) );

	// check size of userinfo + ip before adding it
	if( strlen( Cmd_Argv( 4 ) ) + strlen( NET_AddressToString( &net_from ) ) >= MAX_INFO_STRING )
	{
		Netchan_OutOfBandPrint( NS_SERVER, adr, "reject\n%i\n%i\nUserinfo string too large\n", DROP_TYPE_GENERAL );
		Com_Printf( "ClientConnect: userinfo string exceeded max valid size. Connection refused.\n" );
		return;
	}

	Q_strncpyz( userinfo, Cmd_Argv( 4 ), sizeof( userinfo ) );

	// check for empty userinfo
	if( !( *userinfo ) )
	{
		Netchan_OutOfBandPrint( NS_SERVER, adr, "reject\n%i\n%i\nInvalid userinfo string\n", DROP_TYPE_GENERAL, 0 );
		Com_Printf( "ClientConnect: Empty userinfo string. Connection refused.\n" );
		return;
	}

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey( userinfo, "ip", NET_AddressToString( &net_from ) );

	// see if the challenge is valid
	for( i = 0; i < MAX_CHALLENGES; i++ )
	{
		if( NET_CompareBaseAdr( &net_from, &svs.challenges[i].adr ) )
		{
			if( challenge == svs.challenges[i].challenge )
			{
				svs.challenges[i].challenge = 0; // from r1q2 : reset challenge
				break;
			}
			Netchan_OutOfBandPrint( NS_SERVER, adr, "reject\n%i\n%i\nBad challenge\n",
			                        DROP_TYPE_GENERAL, DROP_FLAG_AUTORECONNECT );
			return;
		}
	}
	if( i == MAX_CHALLENGES )
	{
		Netchan_OutOfBandPrint( NS_SERVER, adr, "reject\n%i\n%i\nNo challenge for address\n",
		                        DROP_TYPE_GENERAL, DROP_FLAG_AUTORECONNECT );
		return;
	}

	newcl = NULL;

	// if there is already a slot for this ip, reuse it
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == CS_FREE )
			continue;
		if( NET_CompareBaseAdr( &adr, &cl->netchan.remoteAddress )
		   && ( cl->netchan.qport == qport || adr.port == cl->netchan.remoteAddress.port ) )
		{
			if( !NET_IsLocalAddress( &adr ) && ( svs.realtime - cl->lastconnect ) < (unsigned)( sv_reconnectlimit->integer * 1000 ) )
			{
				Com_DPrintf( "%s:reconnect rejected : too soon\n", NET_AddressToString( &adr ) );
				return;
			}
			Com_Printf( "%s:reconnect\n", NET_AddressToString( &adr ) );
			newcl = cl;
			break;
		}
	}

	// find a client slot
	if( !newcl )
	{
		for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
		{
			if( cl->state == CS_FREE )
			{
				newcl = cl;
				break;
			}
			// overwrite fakeclient if no free spots found
			if( cl->state && cl->fakeClient )
				newcl = cl;
		}
		if( !newcl )
		{
			Netchan_OutOfBandPrint( NS_SERVER, adr, "reject\n%i\n%i\nServer is full\n",
			                        DROP_TYPE_GENERAL, DROP_FLAG_AUTORECONNECT );
			Com_DPrintf( "Server is full. Rejected a connection.\n" );
			return;
		}
		if( newcl->state && newcl->fakeClient )
			SV_DropClient( newcl, DROP_TYPE_GENERAL, "Making room for a real player." );
	}

	// get the game a chance to reject this connection or modify the userinfo
	if( !SV_ClientConnect( newcl, userinfo, qport, challenge, qfalse ) )
	{
		char *rejtypeflag, *rejmsg;

		// hax because Info_ValueForKey can only be called twice in a row
		rejtypeflag = va( "%s\n%s", Info_ValueForKey( userinfo, "rejtype" ), Info_ValueForKey( userinfo, "rejflag" ) );
		rejmsg = Info_ValueForKey( userinfo, "rejmsg" );

		Netchan_OutOfBandPrint( NS_SERVER, adr, "reject\n%s\n%s\n", rejtypeflag, rejmsg );

		Com_DPrintf( "Game rejected a connection.\n" );
		return;
	}

	// send the connect packet to the client
	Netchan_OutOfBandPrint( NS_SERVER, adr, "client_connect" );
}

//==================
//SVC_FakeConnect
// (Not a real out of band command)
// A connection request that came from the game module
//==================
int SVC_FakeConnect( char *fakeUserinfo, char *fakeIP )
{
	int i;
	char userinfo[MAX_INFO_STRING];
	client_t *cl, *newcl;

	Com_DPrintf( "SVC_FakeConnect ()\n" );

	if( !fakeUserinfo )
		fakeUserinfo = "";
	if( !fakeIP )
		fakeIP = "127.0.0.1";

	Q_strncpyz( userinfo, fakeUserinfo, sizeof( userinfo ) );

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey( userinfo, "ip", fakeIP );

	// find a client slot
	newcl = NULL;
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state == CS_FREE )
		{
			newcl = cl;
			break;
		}
	}
	if( !newcl )
	{
		Com_DPrintf( "Rejected a connection.\n" );
		return -1;
	}

	// get the game a chance to reject this connection or modify the userinfo
	if( !SV_ClientConnect( newcl, userinfo, -1, -1, qtrue ) )
	{
		Com_DPrintf( "Game rejected a connection.\n" );
		return -1;
	}

	// directly call the game begin function
	newcl->state = CS_SPAWNED;
	ge->ClientBegin( CLIENTNUM( newcl ) );

	return CLIENTNUM( newcl );
}

//===============
//Rcon_Validate
//===============
static int Rcon_Validate( void )
{
	if( !strlen( rcon_password->string ) )
		return 0;

	if( strcmp( Cmd_Argv( 1 ), rcon_password->string ) )
		return 0;

	return 1;
}

//===============
//SVC_RemoteCommand
//
//A client issued an rcon command.
//Shift down the remaining args
//Redirect all printfs
//===============
static void SVC_RemoteCommand( msg_t *msg )
{
	int i;
	char remaining[1024];

	i = Rcon_Validate();

	if( !msg || !msg->data || msg->cursize < 5 )
	{
		Com_Printf( "Empty rcon from %s:\n", NET_AddressToString( &net_from ) );
	}

	if( i == 0 )
		Com_Printf( "Bad rcon from %s:\n%s\n", NET_AddressToString( &net_from ), msg->data+4 );
	else
		Com_Printf( "Rcon from %s:\n%s\n", NET_AddressToString( &net_from ), msg->data+4 );

	Com_BeginRedirect( RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect );

	if( !Rcon_Validate() )
	{
		Com_Printf( "Bad rcon_password.\n" );
	}
	else
	{
		remaining[0] = 0;

		for( i = 2; i < Cmd_Argc(); i++ )
		{
			Q_strncatz( remaining, "\"", sizeof( remaining ) );
			Q_strncatz( remaining, Cmd_Argv( i ), sizeof( remaining ) );
			Q_strncatz( remaining, "\" ", sizeof( remaining ) );
		}

		Cmd_ExecuteString( remaining );
	}

	Com_EndRedirect();
}

//=================
//SV_ConnectionlessPacket
//
//A connectionless packet has four leading 0xff
//characters to distinguish it from a game channel.
//Clients that are in the game can still send
//connectionless packets.
//=================
void SV_ConnectionlessPacket( msg_t *msg )
{
	char *s;
	char *c;

	MSG_BeginReading( msg );
	MSG_ReadLong( msg );    // skip the -1 marker

	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );

	c = Cmd_Argv( 0 );
	Com_DPrintf( "Packet %s : %s\n", NET_AddressToString( &net_from ), c );

	if( !Q_stricmp( c, "ping" ) )
		SVC_Ping();
	else if( !Q_stricmp( c, "ack" ) )
		SVC_Ack();
	else if( !Q_stricmp( c, "info" ) )
		SVC_InfoResponse();
	else if( !Q_stricmp( c, "getinfo" ) )
		SVC_GetInfoResponse();
	else if( !Q_stricmp( c, "getstatus" ) )
		SVC_GetStatusResponse();
	else if( !Q_stricmp( c, "getchallenge" ) )
		SVC_GetChallenge();
	else if( !Q_stricmp( c, "connect" ) )
		SVC_DirectConnect();
	else if( !Q_stricmp( c, "rcon" ) )
		SVC_RemoteCommand( msg );
	else
		Com_DPrintf( "bad connectionless packet from %s:\n%s\n", NET_AddressToString( &net_from ), s );
}
