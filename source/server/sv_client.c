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
// sv_client.c -- server code for moving users

#include "server.h"


//============================================================================
//
//		CLIENT
//
//============================================================================

static void SV_ClientResetCommandBuffers( client_t *client )
{

	// reset the reliable commands buffer
	client->clientCommandExecuted = 0;
	client->reliableAcknowledge = 0;
	client->reliableSequence = 0;
	client->reliableSent = 0;
	memset( client->reliableCommands, 0, sizeof( client->reliableCommands ) );

	// reset the usercommands buffer(clc_move)
	client->UcmdTime = 0;
	client->UcmdExecuted = 0;
	client->UcmdReceived = 0;
	memset( client->ucmds, 0, sizeof( client->ucmds ) );

	// do not delta compress the first snap
	client->snapAcknowledged = 0; // zero forces delta compression off
	client->noDeltaSnap = qfalse; // set by client request. Could be set to true here, but there's no need
}


//==================
//SV_ClientConnect
// accept the new client
// this is the only place a client_t is ever initialized
//==================
qboolean SV_ClientConnect( client_t *client, char *userinfo, int qport, int challenge, qboolean fakeClient )
{
	// get the game a chance to reject this connection or modify the userinfo
	if( !ge->ClientConnect( CLIENTNUM( client ), userinfo, fakeClient ) )
		return qfalse;

	// the connection is accepted, set up the client slot
	memset( client, 0, sizeof( *client ) );
	client->challenge = challenge; // save challenge for checksumming

	// parse some info from the info strings
	Q_strncpyz( client->userinfo, userinfo, sizeof( client->userinfo ) );
	SV_UserinfoChanged( client );

	SV_ClientResetCommandBuffers( client );

	// reset timeouts
	client->lastPacketReceivedTime = svs.realtime;
	client->lastconnect = svs.realtime;

	// init the connection
	client->state = CS_CONNECTING;

	client->fakeClient = fakeClient;
	if( client->fakeClient )
	{
		client->netchan.remoteAddress.type = NA_NOTRANSMIT; // fake-clients can't transmit
	}
	else
	{
		Netchan_Setup( NS_SERVER, &client->netchan, net_from, qport );
	}

	return qtrue;
}

//=====================
//SV_DropClient
//
//Called when the player is totally leaving the server, either willingly
//or unwillingly.  This is NOT called if the entire server is quiting
//or crashing.
//=====================
void SV_DropClient( client_t *drop, int type, const char *format, ... )
{
	va_list	argptr;
	char string[1024];

	va_start( argptr, format );
	Q_vsnprintfz( string, sizeof( string ), format, argptr );
	va_end( argptr );

	if( dedicated->integer )
		Com_Printf( "SV_DropClient: \"%s\"\n", string );

	// add the disconnect
	if( drop->fakeClient )
	{
		ge->ClientDisconnect( CLIENTNUM( drop ) );
		SV_ClientResetCommandBuffers( drop ); // make sure everything is clean
		drop->fakeClient = qfalse;
	}
	else
	{
		SV_InitClientMessage( drop, &tmpMessage, NULL, 0 );
		SV_SendServerCommand( drop, "disconnect %i \"%s\"", type, string );
		SV_AddReliableCommandsToMessage( drop, &tmpMessage );

		SV_SendMessageToClient( drop, &tmpMessage );
		Netchan_PushAllFragments( &drop->netchan );

		if( drop->state >= CS_CONNECTED )
		{
			// call the prog function for removing a client
			// this will remove the body, among other things
			ge->ClientDisconnect( CLIENTNUM( drop ) );
		}
	}

	if( drop->download.name )
	{
		if( drop->download.data )
		{
			FS_FreeBaseFile( drop->download.data );
			drop->download.data = NULL;
		}

		Mem_ZoneFree( drop->download.name );
		drop->download.name = NULL;

		drop->download.size = 0;
		drop->download.timeout = 0;
	}

	drop->state = CS_ZOMBIE;    // become free in a few seconds
	drop->name[0] = 0;
}


/*
   ============================================================

   CLIENT COMMAND EXECUTION

   ============================================================
 */

//================
//SV_New_f
//
//Sends the first message from the server to a connected client.
//This will be sent on the initial connection and upon each server load.
//================
static void SV_New_f( client_t *client )
{
	int playernum;
	unsigned int numpure;
	purelist_t *purefile;
	int sv_bitflags = 0;

	Com_DPrintf( "New() from %s\n", client->name );

	// if in CS_AWAITING we have sent the response packet the new once already,
	// but client might have not got it so we send it again
	if( client->state >= CS_SPAWNED )
	{
		Com_Printf( "New not valid -- already spawned\n" );
		return;
	}

	if( sv.state != ss_game )
	{
		Com_Printf( "New not valid -- server not yet ready\n" );
		return;
	}

	playernum = client - svs.clients;

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	SV_InitClientMessage( client, &tmpMessage, NULL, 0 );

	// send the serverdata
	MSG_WriteByte( &tmpMessage, svc_serverdata );
	MSG_WriteLong( &tmpMessage, APP_PROTOCOL_VERSION );
	MSG_WriteLong( &tmpMessage, svs.spawncount );
	MSG_WriteShort( &tmpMessage, (unsigned short)svc.snapFrameTime );
	MSG_WriteString( &tmpMessage, FS_BaseGameDirectory() );
	MSG_WriteString( &tmpMessage, FS_GameDirectory() );
	MSG_WriteShort( &tmpMessage, sv_maxclients->integer );
	MSG_WriteShort( &tmpMessage, playernum );
	MSG_WriteString( &tmpMessage, sv.mapname );

	if( sv_pure->integer )
		sv_bitflags |= SV_BITFLAGS_PURE;

	MSG_WriteByte( &tmpMessage, sv_bitflags );

	// always write purelist
	numpure = Com_CountPureListFiles( svs.purelist );
	if( numpure > (short)0x7fff )
		Com_Error( ERR_DROP, "Error: Too many pure files." );

	MSG_WriteShort( &tmpMessage, numpure );

	purefile = svs.purelist;
	while( purefile )
	{
		MSG_WriteString( &tmpMessage, purefile->filename );
		MSG_WriteLong( &tmpMessage, purefile->checksum );
		purefile = purefile->next;
	}

	SV_ClientResetCommandBuffers( client );

	SV_SendMessageToClient( client, &tmpMessage );
	Netchan_PushAllFragments( &client->netchan );

	// don't let it send reliable commands until we get the first configstring request
	client->state = CS_CONNECTING;
}

//==================
//SV_Configstrings_f
//==================
static void SV_Configstrings_f( client_t *client )
{
	int start;

	if( client->state == CS_CONNECTING )
	{
		Com_DPrintf( "Start Configstrings() from %s\n", client->name );
		client->state = CS_CONNECTED;
	}
	else
		Com_DPrintf( "Configstrings() from %s\n", client->name );

	if( client->state != CS_CONNECTED )
	{
		Com_Printf( "configstrings not valid -- already spawned\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if( atoi( Cmd_Argv( 1 ) ) != svs.spawncount )
	{
		Com_Printf( "SV_Configstrings_f from different level\n" );
		SV_SendServerCommand( client, "reconnect" );
		return;
	}

	start = atoi( Cmd_Argv( 2 ) );
	if( start < 0 )
	{
		start = 0;
	}

	// write a packet full of data
	while( start < MAX_CONFIGSTRINGS &&
	       client->reliableSequence - client->reliableAcknowledge < MAX_RELIABLE_COMMANDS - 8 )
	{
		if( sv.configstrings[start][0] )
			SV_SendServerCommand( client, "cs %i \"%s\"", start, sv.configstrings[start] );
		
		start++;
	}

	// send next command
	if( start == MAX_CONFIGSTRINGS )
		SV_SendServerCommand( client, "cmd baselines %i 0", svs.spawncount );
	else
		SV_SendServerCommand( client, "cmd configstrings %i %i", svs.spawncount, start );
}

//==================
//SV_Baselines_f
//==================
static void SV_Baselines_f( client_t *client )
{
	int start;

	Com_DPrintf( "Baselines() from %s\n", client->name );

	if( client->state != CS_CONNECTED )
	{
		Com_Printf( "baselines not valid -- already spawned\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if( atoi( Cmd_Argv( 1 ) ) != svs.spawncount )
	{
		Com_Printf( "SV_Baselines_f from different level\n" );
		SV_New_f( client );
		return;
	}

	start = atoi( Cmd_Argv( 2 ) );
	if( start < 0 )
		start = 0;

	// write a packet full of data
	SV_InitClientMessage( client, &tmpMessage, NULL, 0 );

	while( tmpMessage.cursize < FRAGMENT_SIZE * 3 && start < MAX_EDICTS )
	{
		if( Com_EntityIsBaseLined( &sv.snapsData.baselines[start] ) )
		{
			MSG_WriteByte( &tmpMessage, svc_spawnbaseline );
			SV_WriteDeltaEntity( &tmpMessage, start, 0, 0, qtrue );
		}
		start++;
	}

	// send next command
	if( start == MAX_EDICTS )
		SV_SendServerCommand( client, "precache %i", svs.spawncount );
	else
		SV_SendServerCommand( client, "cmd baselines %i %i", svs.spawncount, start );

	SV_AddReliableCommandsToMessage( client, &tmpMessage );
	SV_SendMessageToClient( client, &tmpMessage );
}

//==================
//SV_Begin_f
//==================
static void SV_Begin_f( client_t *client )
{
	Com_DPrintf( "Begin() from %s\n", client->name );

	// from r1q2[start] : could be abused to respawn or cause spam/other mod-specific problems
	if( client->state != CS_CONNECTED )
	{
		if( dedicated->integer )
			Com_Printf( "SV_Begin_f: 'Begin' from already spawned client: %s.\n", client->name );
		SV_DropClient( client, DROP_TYPE_GENERAL, "Already spawned client" );
		return;
	}
	// r1q2[end]

	// handle the case of a level changing while a client was connecting
	if( atoi( Cmd_Argv( 1 ) ) != svs.spawncount )
	{
		Com_Printf( "SV_Begin_f from different level\n" );
		SV_SendServerCommand( client, "changing" );
		SV_SendServerCommand( client, "reconnect" );
		return;
	}

	client->state = CS_SPAWNED;

	// call the game begin function
	ge->ClientBegin( CLIENTNUM( client ) );
}

//=============================================================================


//==================
//SV_NextDownload_f
//
//Responds to reliable nextdl packet with unreliable download packet
//If nextdl packet's offet information is negative, download will be stopped
//==================
static void SV_NextDownload_f( client_t *client )
{
	int blocksize;
	int offset;

	if( !client->download.name )
	{
		Com_Printf( "nextdl message for client with no download active, from: %s\n", client->name );
		return;
	}

	if( Q_stricmp( client->download.name, Cmd_Argv( 1 ) ) )
	{
		Com_Printf( "nextdl message for wrong filename, from: %s\n", client->name );
		return;
	}

	offset = atoi( Cmd_Argv( 2 ) );

	if( offset > client->download.size )
	{
		Com_Printf( "nextdl message with too big offset, from: %s\n", client->name );
		return;
	}

	if( offset == -1 )
	{
		Com_Printf( "Upload of %s to %s%s completed\n", client->download.name, client->name, S_COLOR_WHITE );
		if( client->download.data )
		{
			FS_FreeBaseFile( client->download.data );
			client->download.data = NULL;
		}
		Mem_ZoneFree( client->download.name );
		client->download.name = NULL;
		client->download.size = 0;
		client->download.timeout = 0;
		return;
	}

	if( offset < 0 )
	{
		Com_Printf( "Upload of %s to %s%s failed\n", client->download.name, client->name, S_COLOR_WHITE );
		if( client->download.data )
		{
			FS_FreeBaseFile( client->download.data );
			client->download.data = NULL;
		}
		Mem_ZoneFree( client->download.name );
		client->download.name = NULL;
		client->download.size = 0;
		client->download.timeout = 0;
		return;
	}

	if( !client->download.data )
	{
		Com_Printf( "Starting server upload of %s to %s\n", client->download.name, client->name );

		FS_LoadBaseFile( client->download.name, (void **)&client->download.data, NULL, 0 );
		if( !client->download.data )
		{
			Com_Printf( "Error loading %s for uploading\n", client->download.name );
			Mem_ZoneFree( client->download.name );
			client->download.name = NULL;
			client->download.size = 0;
			client->download.timeout = 0;
			return;
		}
	}

	SV_InitClientMessage( client, &tmpMessage, NULL, 0 );
	SV_AddReliableCommandsToMessage( client, &tmpMessage );

	blocksize = client->download.size - offset;
	if( blocksize > FRAGMENT_SIZE * 2 )
		blocksize = FRAGMENT_SIZE * 2;
	if( offset + blocksize > client->download.size )
		blocksize = client->download.size - offset;

	MSG_WriteByte( &tmpMessage, svc_download );
	MSG_WriteString( &tmpMessage, client->download.name );
	MSG_WriteLong( &tmpMessage, offset );
	MSG_WriteLong( &tmpMessage, blocksize );
	MSG_CopyData( &tmpMessage, client->download.data + offset, blocksize );

	SV_SendMessageToClient( client, &tmpMessage );

	client->download.timeout = svs.realtime + 10000;
}

//==================
//SV_GameAllowDownload
//Asks game function whether to allow downloading of a file
//==================
static qboolean SV_GameAllowDownload( client_t *client, const char *requestname, const char *uploadname )
{
	if( client->state < CS_SPAWNED )
		return qfalse;

	return ge->AllowDownload( CLIENTNUM( client ), requestname, uploadname );
}

//==================
//SV_DenyDownload
//Helper function for generating initdownload packets for denying download
//==================
static void SV_DenyDownload( client_t *client, const char *reason )
{
	// size -1 is used to signal that it's refused
	// URL field is used for deny reason
	SV_InitClientMessage( client, &tmpMessage, NULL, 0 );
	SV_SendServerCommand( client, "initdownload \"%s\" %i %u %i \"%s\"", "", -1, 0, qfalse, reason ? reason : "" );
	SV_AddReliableCommandsToMessage( client, &tmpMessage );
	SV_SendMessageToClient( client, &tmpMessage );
}

//==================
//SV_BeginDownload_f
//Responds to reliable download packet with reliable initdownload packet
//==================
static void SV_BeginDownload_f( client_t *client )
{
	const char *requestname;
	const char *uploadname;
	size_t alloc_size;
	unsigned checksum;
	char *url;
	qboolean allow, requestpak;

	if( !sv_uploads->integer || ( !sv_uploads_from_server->integer && ( strlen( sv_uploads_baseurl->string ) == 0 ) ) )
	{
		SV_DenyDownload( client, "Downloading is not allowed on this server" );
		return;
	}

	requestpak = ( atoi( Cmd_Argv( 1 ) ) == 1 );
	requestname = Cmd_Argv( 2 );

	if( !requestname[0] || !COM_ValidateRelativeFilename( requestname ) )
	{
		SV_DenyDownload( client, "Invalid filename" );
		return;
	}

	if( FS_CheckPakExtension( requestname ) )
	{
		if( !requestpak )
		{
			SV_DenyDownload( client, "Pak file requested as a non pak file" );
			return;
		}

		if( FS_FOpenBaseFile( requestname, NULL, FS_READ ) == -1 )
		{
			SV_DenyDownload( client, "File not found" );
			return;
		}

		uploadname = requestname;
	}
	else
	{
		if( FS_FOpenFile( requestname, NULL, FS_READ ) == -1 )
		{
			SV_DenyDownload( client, "File not found" );
			return;
		}

		// check if file is inside a PAK
		if( requestpak )
		{
			uploadname = FS_PakNameForFile( requestname );
			if( !uploadname )
			{
				SV_DenyDownload( client, "File not available in pack" );
				return;
			}
		}
		else
		{
			uploadname = FS_BaseNameForFile( requestname );
			if( !uploadname )
			{
				SV_DenyDownload( client, "File only available in pack" );
				return;
			}
		}
	}

	if( FS_CheckPakExtension( uploadname ) )
	{
		allow = qfalse;

		// allow downloading paks from the pure list, if not spawned
		if( client->state < CS_SPAWNED )
		{
			purelist_t *purefile;

			purefile = svs.purelist;
			while( purefile )
			{
				if( !strcmp( uploadname, purefile->filename ) )
				{
					allow = qtrue;
					break;
				}
				purefile = purefile->next;
			}
		}

		// game module has a change to allow extra downloads
		if( !allow && !SV_GameAllowDownload( client, requestname, uploadname ) )
		{
			SV_DenyDownload( client, "Downloading of this file is not allowed" );
			return;
		}
	}
	else
	{
		if( !SV_GameAllowDownload( client, requestname, uploadname ) )
		{
			SV_DenyDownload( client, "Downloading of this file is not allowed" );
			return;
		}
	}

	// we will just overwrite old download, if any
	if( client->download.name )
	{
		if( client->download.data )
		{
			FS_FreeBaseFile( client->download.data );
			client->download.data = NULL;
		}

		Mem_ZoneFree( client->download.name );
		client->download.name = NULL;

		client->download.size = 0;
		client->download.timeout = 0;
	}

	client->download.size = FS_LoadBaseFile( uploadname, NULL, NULL, 0 );
	if( client->download.size == -1 )
	{
		Com_Printf( "Error getting size of %s for uploading\n", uploadname );
		client->download.size = 0;
		SV_DenyDownload( client, "Error getting file size" );
		return;
	}

	checksum = FS_ChecksumBaseFile( uploadname );

	client->download.timeout = svs.realtime + 1000 * 60 * 60; // this is web download timeout

	alloc_size = sizeof( char ) * ( strlen( uploadname ) + 1 );
	client->download.name = Mem_ZoneMalloc( alloc_size );
	Q_strncpyz( client->download.name, uploadname, alloc_size );

	Com_Printf( "Offering %s to %s\n", client->download.name, client->name );

	// only .pk3 from web atm.
	if( FS_CheckPakExtension( uploadname ) && sv_uploads_baseurl->string[0] != 0 )
	{
		alloc_size = sizeof( char ) * ( strlen( sv_uploads_baseurl->string ) + 1 );
		url = Mem_TempMalloc( alloc_size );
		Q_snprintfz( url, alloc_size, "%s/", sv_uploads_baseurl->string );
	}
	else
	{
		url = NULL;
	}

	// start the download
	SV_InitClientMessage( client, &tmpMessage, NULL, 0 );
	SV_SendServerCommand( client, "initdownload \"%s\" %i %u %i \"%s\"", client->download.name,
	                     client->download.size, checksum, ( sv_uploads_from_server->integer != 0 ), ( url ? url : "" ) );
	SV_AddReliableCommandsToMessage( client, &tmpMessage );
	SV_SendMessageToClient( client, &tmpMessage );

	if( url )
	{
		Mem_TempFree( url );
		url = NULL;
	}
}


//============================================================================


//=================
//SV_Disconnect_f
//The client is going to disconnect, so remove the connection immediately
//=================
static void SV_Disconnect_f( client_t *client )
{
	SV_DropClient( client, DROP_TYPE_GENERAL, "User disconnected" );
}


//==================
//SV_ShowServerinfo_f
//Dumps the serverinfo info string
//==================
static void SV_ShowServerinfo_f( client_t *client )
{
	Info_Print( Cvar_Serverinfo() );
}

//==================
//SV_UserinfoCommand_f
//==================
static void SV_UserinfoCommand_f( client_t *client )
{
	char *uinfo;
	uinfo = Cmd_Argv( 1 );

	if( Cmd_Argc() > 2 )
	{                  // invalid ?
		Com_DPrintf( "WARNING: SV_ParseUserinfoCommand_f: userinfo string with more than 1 tokem received from client %s\n", client->name );
	}

	// empty userinfo received
	if( !uinfo || !uinfo[0] )
	{
		if( !strlen( client->userinfo ) )
			SV_DropClient( client, DROP_TYPE_GENERAL, "Empty userinfo received" );

		return;
	}

	Q_strncpyz( client->userinfo, uinfo, sizeof( client->userinfo ) );
	SV_UserinfoChanged( client );
}

typedef struct
{
	char *name;
	void ( *func )( client_t *client );
} ucmd_t;

ucmd_t ucmds[] =
{
	// auto issued
	{ "new", SV_New_f },
	{ "configstrings", SV_Configstrings_f },
	{ "baselines", SV_Baselines_f },
	{ "begin", SV_Begin_f },
	{ "disconnect", SV_Disconnect_f },
	{ "usri", SV_UserinfoCommand_f },

	// issued by hand at client consoles
	{ "info", SV_ShowServerinfo_f },

	{ "download", SV_BeginDownload_f },
	{ "nextdl", SV_NextDownload_f },

	{ NULL, NULL }
};

//==================
//SV_ExecuteUserCommand
//==================
static void SV_ExecuteUserCommand( client_t *client, char *s )
{
	ucmd_t *u;

	Cmd_TokenizeString( s );

	for( u = ucmds; u->name; u++ )
	{
		if( !strcmp( Cmd_Argv( 0 ), u->name ) )
		{
			u->func( client );
			break;
		}
	}

	// from r1q2: don't pass commands to game before being fully connected
	if( client->state < CS_SPAWNED )
		return;

	if( !u->name && sv.state == ss_game )
		ge->ClientCommand( CLIENTNUM( client ) );
}

/*
   ===========================================================================

   USER CMD EXECUTION

   ===========================================================================
 */

//===================
//SV_FindNextUserCommand - Returns the next valid usercmd_t in execution list
//===================
usercmd_t *SV_FindNextUserCommand( client_t *client )
{
	usercmd_t *ucmd;
	unsigned int higherTime = 0xFFFFFFFF;
	unsigned int i;

	higherTime = svs.gametime; // ucmds can never have a higher timestamp than server time, unless cheating
	ucmd = NULL;
	if( client )
	{
		for( i = client->UcmdExecuted + 1; i <= client->UcmdReceived; i++ )
		{
			// skip backups if already executed
			if( client->UcmdTime >= client->ucmds[i & CMD_MASK].serverTimeStamp )
				continue;

			if( client->ucmds[i & CMD_MASK].serverTimeStamp < higherTime )
			{
				higherTime = client->ucmds[i & CMD_MASK].serverTimeStamp;
				ucmd = &client->ucmds[i & CMD_MASK];
			}
		}
	}

	return ucmd;
}

//===================
//SV_ExecuteClientThinks - Execute all pending usercmd_t
//===================
void SV_ExecuteClientThinks( int clientNum )
{
	unsigned int msec, minUcmdTime;
	int timeDelta;
	client_t *client;
	usercmd_t *ucmd;

	if( clientNum >= sv_maxclients->integer || clientNum < 0 )
		return;

	client = svs.clients + clientNum;
	if( client->state < CS_SPAWNED )
		return;

	if( client->fakeClient )
		return;

	// don't let client command time delay too far away in the past
	minUcmdTime = ( svs.gametime > 999 ) ? ( svs.gametime - 999 ) : 0;
	if( client->UcmdTime < minUcmdTime )
		client->UcmdTime = minUcmdTime;

	while( ( ucmd = SV_FindNextUserCommand( client ) ) != NULL )
	{
		msec = ucmd->serverTimeStamp - client->UcmdTime;
		clamp( msec, 1, 200 );
		ucmd->msec = msec;
		// convert push fractions to push times
		ucmd->forwardmove = ucmd->forwardfrac * msec;
		ucmd->sidemove = ucmd->sidefrac * msec;
		ucmd->upmove = ucmd->upfrac * msec;
		timeDelta = 0;
		if( client->snapAcknowledged > 0 )
			timeDelta = -(int)( svs.gametime - ucmd->serverTimeStamp );

		ge->ClientThink( CLIENTNUM( client ), ucmd, timeDelta );

		client->UcmdTime = ucmd->serverTimeStamp;
	}

	// we did the entire update
	client->UcmdExecuted = client->UcmdReceived;
}

//==================
//MSG_ReadDeltaUsercmd
//==================
static void MSG_ReadDeltaUsercmd( msg_t *msg, usercmd_t *from, usercmd_t *move )
{
	int bits;

	memcpy( move, from, sizeof( *move ) );

	bits = MSG_ReadByte( msg );

	// read current angles
	if( bits & CM_ANGLE1 )
		move->angles[0] = MSG_ReadShort( msg );
	if( bits & CM_ANGLE2 )
		move->angles[1] = MSG_ReadShort( msg );
	if( bits & CM_ANGLE3 )
		move->angles[2] = MSG_ReadShort( msg );

	// read movement
	if( bits & CM_FORWARD )
		move->forwardfrac = (float)MSG_ReadChar( msg )/UCMD_PUSHFRAC_SNAPSIZE;
	if( bits & CM_SIDE )
		move->sidefrac = (float)MSG_ReadChar( msg )/UCMD_PUSHFRAC_SNAPSIZE;
	if( bits & CM_UP )
		move->upfrac = (float)MSG_ReadChar( msg )/UCMD_PUSHFRAC_SNAPSIZE;

	// read buttons
	if( bits & CM_BUTTONS )
		move->buttons = MSG_ReadByte( msg );

	move->serverTimeStamp = MSG_ReadLong( msg );
}

//==================
//SV_ParseMoveCommand
//==================
static void SV_ParseMoveCommand( client_t *client, msg_t *msg )
{
	unsigned int i;
	usercmd_t nullcmd;
	qbyte bitflags;

	unsigned int ucmdFirst;
	unsigned int ucmdHead;
	unsigned int ucmdCount;

	bitflags = (qbyte)MSG_ReadByte( msg );

	client->noDeltaSnap = ( bitflags & 1 ) ? qtrue : qfalse;
	client->snapAcknowledged = (unsigned int)MSG_ReadLong( msg );

	// if not spawned we are getting acknowledges from invalid levels
	if( client->state != CS_SPAWNED )
		client->snapAcknowledged = 0;

	// read the id of the first ucmd we will receive
	ucmdHead = (unsigned int)MSG_ReadLong( msg );

	// read the number of ucmds we will receive
	ucmdCount = (unsigned int)MSG_ReadByte( msg );

	if( ucmdCount > CMD_MASK )
	{
		SV_DropClient( client, DROP_TYPE_GENERAL, "Ucmd overflow" );
		return;
	}

	ucmdFirst = ucmdHead > ucmdCount ? ucmdHead - ucmdCount : 0;
	client->UcmdReceived = ucmdHead < 1 ? 0 : ucmdHead - 1;

	// read the user commands
	for( i = ucmdFirst; i < ucmdHead; i++ )
	{
		if( i == ucmdFirst )
		{              // first one isn't delta compressed
			memset( &nullcmd, 0, sizeof( nullcmd ) );
			MSG_ReadDeltaUsercmd( msg, &nullcmd, &client->ucmds[i & CMD_MASK] );
		}
		else
		{
			MSG_ReadDeltaUsercmd( msg, &client->ucmds[( i-1 ) & CMD_MASK], &client->ucmds[i & CMD_MASK] );
		}
	}
}


#define	MAX_STRINGCMDS	8
#define	MAX_USERINFO_UPDATES	8   // from r1q2 : limit userinfo update per packet

//===================
//SV_ParseClientMessage
//The current message is parsed for the given client
//===================
void SV_ParseClientMessage( client_t *client, msg_t *msg )
{
	int c;
	char *s;
	qboolean move_issued;
	int userinfoCount;  // from r1q2
	unsigned int cmdNum;

	if( !msg )
		return;

	SV_UpdateActivity();

	// only allow one move command
	move_issued = qfalse;
	userinfoCount = 0; // from r1q2
	while( 1 )
	{
		if( msg->readcount > msg->cursize )
		{
			Com_Printf( "SV_ReadClientMessage: badread\n" );
			SV_DropClient( client, DROP_TYPE_GENERAL, "SV_ReadClientMessage: bad message from client" );
			return;
		}

		c = MSG_ReadByte( msg );
		if( c == -1 )
			break;

		switch( c )
		{
		default:
			Com_Printf( "SV_ReadClientMessage: unknown command char\n" );
			SV_DropClient( client, DROP_TYPE_GENERAL, "SV_ReadClientMessage: unknown command char" );
			return;

		case clc_nop:
			break;

		case clc_move:
			{
				if( move_issued )
					return; // someone is trying to cheat...

				move_issued = qtrue;
				SV_ParseMoveCommand( client, msg );
			}
			break;

		case clc_svcack:
			{
				cmdNum = MSG_ReadLong( msg );
				if( cmdNum < client->reliableAcknowledge || cmdNum > client->reliableSent )
				{
					//SV_DropClient( client, DROP_TYPE_GENERAL, "SV_ExecuteClientMessage: bad server command acknowledged" );
					return;
				}
				client->reliableAcknowledge = cmdNum;
			}
			break;

		case clc_clientcommand:
			{
				cmdNum = MSG_ReadLong( msg );
				if( cmdNum <= client->clientCommandExecuted )
				{
					s = MSG_ReadString( msg ); // read but ignore
					continue;
				}
				client->clientCommandExecuted = cmdNum;
				s = MSG_ReadString( msg );
				SV_ExecuteUserCommand( client, s );
				if( client->state == CS_ZOMBIE )
					return; // disconnect command
			}
			break;

		case clc_extension:
			{
				int ext, len, ver;

				ext = MSG_ReadByte( msg );		// extension id
				ver = MSG_ReadByte( msg );		// version number
				len = MSG_ReadShort( msg );		// command length

				switch( ext )
				{
				default:
					// unsupported
					MSG_SkipData( msg, len );
					break;
				}
			}
			break;
		}
	}
}
