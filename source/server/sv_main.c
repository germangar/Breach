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

mempool_t *sv_mempool;

cvar_t *sv_timeout;            // seconds without any message
cvar_t *sv_zombietime;         // seconds to sink messages after disconnect

cvar_t *rcon_password;         // password for remote server commands

cvar_t *sv_uploads;
cvar_t *sv_uploads_from_server;
cvar_t *sv_uploads_baseurl;

cvar_t *sv_pure;
cvar_t *sv_pure_forcemodulepk3;

cvar_t *sv_maxclients;
cvar_t *sv_showclamp;

cvar_t *sv_hostname;
cvar_t *sv_public;         // should heartbeats be sent
cvar_t *sv_defaultmap;

cvar_t *sv_reconnectlimit; // minimum seconds between connect messages

cvar_t *sv_compresspackets;
cvar_t *sv_masterservers;
cvar_t *sv_skilllevel;

cvar_t *sv_snap_nocull;

cvar_t *sv_debug_serverCmd;

cvar_t *sv_autoUpdate;
cvar_t *sv_lastAutoUpdate;

//============================================================================

//=================
// SV_ProcessPacket
//=================
static qboolean SV_ProcessPacket( netchan_t *netchan, msg_t *msg )
{
	int sequence, sequence_ack;
	int qport = -1;
	int zerror;

	if( !Netchan_Process( netchan, msg ) )
		return qfalse; // wasn't accepted for some reason

	// now if compressed, expand it
	MSG_BeginReading( msg );
	sequence = MSG_ReadLong( msg );
	sequence_ack = MSG_ReadLong( msg );
	qport = MSG_ReadShort( msg );
	if( msg->compressed )
	{
		zerror = Netchan_DecompressMessage( msg );
		if( zerror < 0 )
		{   // compression error. Drop the packet
			Com_DPrintf( "SV_ProcessPacket: Compression error %i. Dropping packet\n", zerror );
			return qfalse;
		}
	}

	return qtrue;
}

//=================
//SV_ReadPackets
//=================
static void SV_ReadPackets( void )
{
	int i;
	client_t *cl;
	int qport;

	static msg_t msg;
	static qbyte msgData[MAX_MSGLEN];

	MSG_Init( &msg, msgData, sizeof( msgData ) );
	MSG_Clear( &msg );

	while( NET_GetPacket( NS_SERVER, &net_from, &msg ) )
	{
		// check for connectionless packet (0xffffffff) first
		if( *(int *)msg.data == -1 )
		{
			SV_ConnectionlessPacket( &msg );
			continue;
		}

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		MSG_BeginReading( &msg );
		MSG_ReadLong( &msg ); // sequence number
		MSG_ReadLong( &msg ); // sequence number
		qport = MSG_ReadShort( &msg ) & 0xffff;

		// data follows

		// check for packets from connected clients
		for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
		{
			if( cl->state == CS_FREE )
				continue;
			if( cl->fakeClient )
				continue;
			if( !NET_CompareBaseAdr( &net_from, &cl->netchan.remoteAddress ) )
				continue;
			if( cl->netchan.qport != qport )
				continue;
			if( cl->netchan.remoteAddress.port != net_from.port )
			{
				Com_Printf( "SV_ReadPackets: fixing up a translated port\n" );
				cl->netchan.remoteAddress.port = net_from.port;
			}

			if( SV_ProcessPacket( &cl->netchan, &msg ) )
			{ // this is a valid, sequenced packet, so process it
				if( cl->state != CS_ZOMBIE )
				{
					cl->lastPacketReceivedTime = svs.realtime; // don't timeout
					SV_ParseClientMessage( cl, &msg );
				}
			}
			break;
		}

		if( i != sv_maxclients->integer )
			continue;
	}
}

//==================
//SV_CheckTimeouts
//
//If a packet has not been received from a client for timeout->value
//seconds, drop the conneciton.  Server frames are used instead of
//realtime to avoid dropping the local client while debugging.
//
//When a client is normally dropped, the client_t goes into a zombie state
//for a few seconds to make sure any final reliable message gets resent
//if necessary
//==================
static void SV_CheckTimeouts( void )
{
	int i;
	client_t *cl;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		// fake clients do not timeout
		if( cl->fakeClient )
			cl->lastPacketReceivedTime = svs.realtime;

		// message times may be wrong across a changelevel
		else if( cl->lastPacketReceivedTime > svs.realtime )
			cl->lastPacketReceivedTime = svs.realtime;

		if( cl->state == CS_ZOMBIE && cl->lastPacketReceivedTime + 1000 * sv_zombietime->value < svs.realtime )
		{
			cl->state = CS_FREE; // can now be reused
			continue;
		}

		if( ( cl->state != CS_FREE && cl->state != CS_ZOMBIE ) &&
		   ( cl->lastPacketReceivedTime + 1000 * sv_timeout->value < svs.realtime ) )
		{
			SV_ClientPrintf( NULL, "%s%s timed out\n", cl->name, S_COLOR_WHITE );
			SV_DropClient( cl, DROP_TYPE_GENERAL, "Connection timed out" );
			cl->state = CS_FREE; // don't bother with zombie state
		}

		// timeout downloads left open
		if( ( cl->state != CS_FREE && cl->state != CS_ZOMBIE ) &&
		   ( cl->download.name && cl->download.timeout < svs.realtime ) )
		{
			Com_Printf( "Download of %s to %s%s timed out\n", cl->download.name, cl->name, S_COLOR_WHITE );

			if( cl->download.data )
			{
				FS_FreeBaseFile( cl->download.data );
				cl->download.data = NULL;
			}

			Mem_ZoneFree( cl->download.name );
			cl->download.name = NULL;

			cl->download.size = 0;
			cl->download.timeout = 0;
		}
	}
}

//#define WORLDFRAMETIME 25 // 40fps
//#define WORLDFRAMETIME 20 // 50fps
#define WORLDFRAMETIME 16 // 62.5fps
//=================
//SV_RunGameFrame
//=================
static qboolean SV_RunGameFrame( int msec )
{
	static unsigned int accTime = 0;
	qboolean refreshSnapshot;
	qboolean refreshGameModule;
	qboolean sentFragments;
	int numEntities;

	accTime += msec;

	refreshSnapshot = qfalse;
	refreshGameModule = qfalse;

	sentFragments = SV_SendClientsFragments();

	// see if it's time to run a new game frame
	if( accTime >= WORLDFRAMETIME )
		refreshGameModule = qtrue;

	// see if it's time for a new snapshot
	if( !sentFragments && ( svs.gametime >= sv.nextSnapTime ) )
	{
		refreshGameModule = qtrue;
		refreshSnapshot = qtrue;
	}

	// if there aren't pending packets to be sent, we can sleep
	if( dedicated->integer && !sentFragments && !refreshSnapshot )
	{
		int sleeptime = min( WORLDFRAMETIME - ( accTime + 1 ), sv.nextSnapTime - ( svs.gametime + 1 ) );

		clamp_low( sleeptime, 0 );

		if( sleeptime > 0 )
			NET_Sleep( sleeptime );
	}

	if( refreshGameModule )
	{
		unsigned int moduleTime;

		if( accTime >= WORLDFRAMETIME )
		{
			moduleTime = WORLDFRAMETIME;
			accTime -= WORLDFRAMETIME;
			if( accTime >= WORLDFRAMETIME ) // don't let it accumulate more than 1 frame
				accTime = WORLDFRAMETIME - 1;
		}
		else
		{
			moduleTime = accTime;
			accTime = 0;
		}

		if( host_speeds->integer )
			time_before_game = Sys_Milliseconds();

		ge->RunFrame( moduleTime, svs.gametime );

		if( host_speeds->integer )
			time_after_game = Sys_Milliseconds();
	}

	if( refreshSnapshot )
	{
		int extraSnapTime;

		// set up for sending a snapshot
		sv.framenum++;
		numEntities = ge->SnapFrame( sv.framenum );
		SV_BackUpSnapshotData( sv.framenum, numEntities );

		// set time for next snapshot
		extraSnapTime = (int)( svs.gametime - sv.nextSnapTime );
		if( extraSnapTime > svc.snapFrameTime * 0.5 ) // don't let too much time be accumulated
			extraSnapTime = svc.snapFrameTime * 0.5;

		sv.nextSnapTime = svs.gametime + ( svc.snapFrameTime - extraSnapTime );

		return qtrue;
	}

	return qfalse;
}

//==================
//SV_CheckDefaultMap
//==================
static void SV_CheckDefaultMap( void )
{
	if( svc.autostarted )
		return;

	svc.autostarted = qtrue;
	if( dedicated->integer )
	{
		if( ( sv.state == ss_dead ) && sv_defaultmap && strlen( sv_defaultmap->string ) && !strlen( sv.mapname ) )
			Cbuf_ExecuteText( EXEC_APPEND, va( "map %s\n", sv_defaultmap->string ) );
	}
}

/*
* SV_UpdateActivity
*/
void SV_UpdateActivity( void )
{
	svc.last_activity = Sys_Milliseconds();
	//Com_Printf( "Server activity\n" );
}

/*
* SV_CheckAutoUpdate
*/
static void SV_CheckAutoUpdate( void )
{
	unsigned int days;

	if( !sv_pure->integer && sv_autoUpdate->integer )
	{
		Com_Printf( "WARNING: Autoupdate is not available for unpure servers.\n" );
		Cvar_ForceSet( "sv_autoUpdate", "0" );
	}

	if( !sv_autoUpdate->integer || !dedicated->integer )
		return;

	// do not if there has been any activity in the last 2 hours
	if( ( svc.last_activity + 1800000 ) > Sys_Milliseconds() )
		return;

	days = (unsigned int)sv_lastAutoUpdate->integer;

	// less that 2 days since last check?
	if( days + 1 < Com_DaysSince1900() )
		SV_AutoUpdateFromWeb( qfalse );
}

//==================
//SV_Frame
//==================
void SV_Frame( int realmsec, int gamemsec )
{
	const unsigned int wrappingPoint = 0x70000000;
	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if( !svs.initialized )
	{
		SV_CheckDefaultMap();
		return;
	}

	svs.realtime += realmsec;
	svs.gametime += gamemsec;

	// restart if the server is running for too long
	if( svs.realtime > wrappingPoint || svs.gametime > wrappingPoint || sv.framenum >= wrappingPoint )
	{
		SV_ClientPrintf( NULL, "Restarting server due to time wrapping" );
		// "map" makes a hard restart, but this would be triggered only in the most extreme case.
		// the game module should have triggered softer cases before.
		Cbuf_AddText( va( "wait; map %s\n", sv.mapname ) );
	}

	// check timeouts
	SV_CheckTimeouts();

	// get packets from clients
	SV_ReadPackets();

	// let everything in the world think and move
	if( SV_RunGameFrame( gamemsec ) )
	{

		// send messages back to the clients that had packets read this frame
		SV_SendClientMessages();

		// send a heartbeat to the master if needed
		SV_MasterHeartbeat();

		// clear teleport flags, etc for next frame
		ge->ClearSnap();
	}

	SV_CheckAutoUpdate();
}


//============================================================================


//=================
//SV_UserinfoChanged
//
//Pull specific info from a newly changed userinfo string
//into a more C friendly form.
//=================
void SV_UserinfoChanged( client_t *cl )
{
	char *val;
	char *s;
	int i;

	// skin
	val = Info_ValueForKey( cl->userinfo, "skin" );
	if( val && strchr( val, '/' ) )
	{
		Com_Printf( "SV_UserinfoChanged: Fixing invalid skin name %s\n", val );
		s = strchr( val, '/' );
		*s = 0;

		Info_SetValueForKey( cl->userinfo, "skin", val );
	}

	// model
	val = Info_ValueForKey( cl->userinfo, "model" );
	if( val && strchr( val, '/' ) )
	{
		Com_Printf( "SV_UserinfoChanged: Fixing invalid model name %s\n", val );
		s = strchr( val, '/' );
		*s = 0;

		Info_SetValueForKey( cl->userinfo, "model", val );
	}

	// call prog code to allow overrides
	ge->ClientUserinfoChanged( CLIENTNUM( cl ), cl->userinfo );

	// r1q2[start] : verify, validate, truncate and print name changes
	val = Info_ValueForKey( cl->userinfo, "name" );
	if( !val || !val[0] )
	{
		Q_strncpyz( cl->name, "Player", sizeof( cl->name ) );
	}
	else
	{
		Q_strncpyz( cl->name, val, sizeof( cl->name ) );

		// mask off high bit
		for( i = 0; i < sizeof( cl->name ); i++ )
			cl->name[i] &= 127;
	}
}


//============================================================================

//===============
//SV_Init
//
//Only called at plat.exe startup, not for each game
//===============
void SV_Init( void )
{
	cvar_t *sv_pps;
	cvar_t *sv_fps;

	SV_InitOperatorCommands();

	sv_mempool = Mem_AllocPool( NULL, "Server" );

	Cvar_Get( "sv_cheats", "0", CVAR_SERVERINFO | CVAR_LATCH );
	Cvar_Get( "protocol", va( "%i", APP_PROTOCOL_VERSION ), CVAR_SERVERINFO | CVAR_NOSET );

	rcon_password =		    Cvar_Get( "rcon_password", "", 0 );
	sv_hostname =		    Cvar_Get( "sv_hostname", APPLICATION " server", CVAR_SERVERINFO | CVAR_ARCHIVE );
	sv_timeout =		    Cvar_Get( "sv_timeout", "125", 0 );
	sv_zombietime =		    Cvar_Get( "sv_zombietime", "2", 0 );
	sv_showclamp =		    Cvar_Get( "sv_showclamp", "0", 0 );

	if( dedicated->integer )
	{
		sv_uploads =		    Cvar_Get( "sv_uploads", "1", CVAR_READONLY );
		sv_uploads_from_server = Cvar_Get( "sv_uploads_from_server", "1", CVAR_READONLY );
		sv_autoUpdate = Cvar_Get( "sv_autoUpdate", "1", CVAR_ARCHIVE );

		sv_pure =		Cvar_Get( "sv_pure", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO );

#ifdef PUBLIC_BUILD
		sv_public =		Cvar_Get( "sv_public", "1", CVAR_ARCHIVE | CVAR_LATCH );
#else
		sv_public =		Cvar_Get( "sv_public", "0", CVAR_ARCHIVE | CVAR_LATCH );
#endif
	}
	else
	{
		sv_uploads =		    Cvar_Get( "sv_uploads", "1", CVAR_ARCHIVE );
		sv_uploads_from_server = Cvar_Get( "sv_uploads_from_server", "1", CVAR_ARCHIVE );
		sv_autoUpdate = Cvar_Get( "sv_autoUpdate", "0", CVAR_READONLY );

		sv_pure =		Cvar_Get( "sv_pure", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO );
		sv_public =		Cvar_Get( "sv_public", "0", CVAR_ARCHIVE );
	}

	sv_uploads_baseurl =	    Cvar_Get( "sv_uploads_baseurl", "", CVAR_ARCHIVE );
	sv_lastAutoUpdate = Cvar_Get( "sv_lastAutoUpdate", "0", CVAR_READONLY|CVAR_ARCHIVE );
	sv_pure_forcemodulepk3 =    Cvar_Get( "sv_pure_forcemodulepk3", "", CVAR_LATCH );
		
	sv_defaultmap =		    Cvar_Get( "sv_defaultmap", "", CVAR_ARCHIVE );
	sv_reconnectlimit =	    Cvar_Get( "sv_reconnectlimit", "3", CVAR_ARCHIVE );
	sv_maxclients =		    Cvar_Get( "sv_maxclients", "1", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH );

	Cvar_Get( "sv_modmanifest", "", CVAR_READONLY );
	Cvar_ForceSet( "sv_modmanifest", "" );

	// fix invalid sv_maxclients values
	if( sv_maxclients->integer < 1 )
		Cvar_FullSet( "sv_maxclients", "1", CVAR_ARCHIVE|CVAR_SERVERINFO|CVAR_LATCH, qtrue );
	else if( sv_maxclients->integer > MAX_CLIENTS )
		Cvar_FullSet( "sv_maxclients", va( "%i", MAX_CLIENTS ), CVAR_ARCHIVE|CVAR_SERVERINFO|CVAR_LATCH, qtrue );

	// jal : cap client's exceding server rules
	sv_compresspackets =	    Cvar_Get( "sv_compresspackets", "1", CVAR_DEVELOPER );
	sv_skilllevel =		    Cvar_Get( "sv_skilllevel", "1", CVAR_SERVERINFO|CVAR_ARCHIVE );
	sv_masterservers =	    Cvar_Get( "masterservers", DEFAULT_MASTER_SERVERS_IPS, CVAR_LATCH );
	sv_debug_serverCmd =	    Cvar_Get( "sv_debug_serverCmd", "0", CVAR_ARCHIVE );

	// this is a message holder for shared use
	MSG_Init( &tmpMessage, tmpMessageData, sizeof( tmpMessageData ) );

	// init server updates ratio
	if( dedicated->integer )
		sv_pps = Cvar_Get( "sv_pps", "20", CVAR_SERVERINFO|CVAR_NOSET );
	else
		sv_pps = Cvar_Get( "sv_pps", "20", CVAR_SERVERINFO );
	svc.snapFrameTime = (int)( 1000 / sv_pps->value );
	if( svc.snapFrameTime > 200 )
	{                           // too slow, also, netcode uses a byte
		Cvar_ForceSet( "sv_pps", "5" );
		svc.snapFrameTime = 200;
	}
	else if( svc.snapFrameTime < 10 )
	{                                 // abusive
		Cvar_ForceSet( "sv_pps", "100" );
		svc.snapFrameTime = 10;
	}

	sv_fps = Cvar_Get( "sv_fps", "62", CVAR_NOSET );
	svc.gameFrameTime = (int)( 1000 / sv_fps->value );
	if( svc.gameFrameTime > svc.snapFrameTime )
	{                                         // gamecode can never be slower than snaps
		svc.gameFrameTime = svc.snapFrameTime;
		Cvar_ForceSet( "sv_fps", sv_pps->dvalue );
	}

	sv_snap_nocull = Cvar_Get( "sv_snap_nocull", "0", CVAR_DEVELOPER );

	Com_Printf( "Game running at %i fps. Server transmit at %i pps\n", sv_fps->integer, sv_pps->integer );

	// init the master servers list
	SV_InitMaster();
}

//==================
//SV_FinalMessage
//
//Used by SV_Shutdown to send a final message to all
//connected clients before the server goes down.  The messages are sent immediately,
//not just stuck on the outgoing message list, because the server is going
//to totally exit after returning from this function.
//==================
void SV_FinalMessage( char *message, qboolean reconnect )
{
	int i, j;
	client_t *cl;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->fakeClient )
			continue;
		if( cl->state >= CS_CONNECTING )
		{
			if( reconnect )
				SV_SendServerCommand( cl, "forcereconnect \"%s\"", message );
			else
				SV_SendServerCommand( cl, "disconnect %i \"%s\"", DROP_TYPE_GENERAL, message );

			SV_InitClientMessage( cl, &tmpMessage, NULL, 0 );
			SV_AddReliableCommandsToMessage( cl, &tmpMessage );
			// send it twice
			for( j = 0; j < 2; j++ )
				SV_SendMessageToClient( cl, &tmpMessage );
		}
	}
}

//================
//SV_Shutdown
//
//Called when each game quits,
//before Sys_Quit or Sys_Error
//================
void SV_Shutdown( char *finalmsg, qboolean reconnect )
{
	if( svs.clients )
		SV_FinalMessage( finalmsg, reconnect );

	SV_ShutdownGameProgs();

	// get any latched variable changes (sv_maxclients, etc)
	Cvar_GetLatchedVars( CVAR_LATCH );

	if( svs.cms )
	{
		CM_Free( svs.cms );
		svs.cms = NULL;
	}

	memset( &sv, 0, sizeof( sv ) );
	Com_SetServerState( sv.state );
	Com_FreePureList( &svs.purelist );

	if( sv_mempool )
		Mem_EmptyPool( sv_mempool );
	memset( &svs, 0, sizeof( svs ) );
}
