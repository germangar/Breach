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
// cl_main.c  -- client main loop

#include "client.h"
#include "../qcommon/webdownload.h"

cvar_t *cl_stereo_separation;
cvar_t *cl_stereo;

cvar_t *rcon_client_password;
cvar_t *rcon_address;

cvar_t *cl_timeout;
cvar_t *cl_maxfps;
#ifdef PUTCPU2SLEEP
cvar_t *cl_sleep;
#endif
cvar_t *cl_extrapolate;
cvar_t *cl_pps;
cvar_t *cl_compresspackets;
cvar_t *cl_shownet;

cvar_t *cl_timedemo;
cvar_t *cl_demoavi_video;
cvar_t *cl_demoavi_audio;
cvar_t *cl_demoavi_fps;
cvar_t *cl_demoavi_scissor;

cvar_t *lookspring;
cvar_t *lookstrafe;

cvar_t *m_pitch;
cvar_t *m_yaw;
cvar_t *m_forward;
cvar_t *m_side;

//
// userinfo
//
cvar_t *info_password;
cvar_t *cl_masterservers;

cvar_t *cl_debug_serverCmd;
cvar_t *cl_debug_timeDelta;

cvar_t *cl_downloads;
cvar_t *cl_downloads_from_web;
cvar_t *cl_downloads_from_web_timeout;
cvar_t *cl_download_allow_modules;

client_static_t	cls;
client_state_t cl;

//======================================================================


/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
* CL_AddReliableCommand
*
* The given command will be transmitted to the server, and is gauranteed to
* not have future usercmd_t executed before it is executed
*/
void CL_AddReliableCommand( /*const*/ char *cmd )
{
	int index;

	if( !cmd || !strlen( cmd ) )
		return;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if( cls.reliableSequence > cls.reliableAcknowledge + MAX_RELIABLE_COMMANDS )
	{
		Com_Error( ERR_DROP, "Client command overflow" );
	}
	cls.reliableSequence++;
	index = cls.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( cls.reliableCommands[index], cmd, sizeof( cls.reliableCommands[index] ) );
}

/*
* CL_UpdateClientCommandsToServer
*
* Add the pending commands to the message
*/
void CL_UpdateClientCommandsToServer( msg_t *msg )
{
	unsigned int i;

	//if( cl_debug_serverCmd->integer & 2 )
	//	Com_Printf( "cls.reliableAcknowledge: %i cls.reliableSequence:%i\n", cls.reliableAcknowledge,cls.reliableSequence );

	// write any unacknowledged clientCommands
	for( i = cls.reliableAcknowledge + 1; i <= cls.reliableSequence; i++ )
	{
		if( !strlen( cls.reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] ) )
			continue;

		MSG_WriteByte( msg, clc_clientcommand );
		MSG_WriteLong( msg, i );
		MSG_WriteString( msg, cls.reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] );

		if( cl_debug_serverCmd->integer & 2 && cls.state < CA_ACTIVE )
			Com_Printf( "CL_UpdateClientCommandsToServer(%i): %s\n", i, cls.reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] );
	}

	cls.reliableSent = cls.reliableSequence;
}

/*
* Cmd_ForwardToServer
*
* adds the current command line as a command to the client message.
* things like godmode, noclip, etc, are commands directed to the server,
* so when they are typed in at the console, they will need to be forwarded.
*/
void Cmd_ForwardToServer( void )
{
	char *cmd;

	cmd = Cmd_Argv( 0 );
	if( cls.state < CA_HANDSHAKE || cls.demo.playing || *cmd == '-' || *cmd == '+' )
	{
		Com_Printf( "Unknown command \"%s\"\n", cmd );
		return;
	}

	CL_AddReliableCommand( va( "%s %s\n", cmd, Cmd_Args() ) );
}

/*
* CL_ForwardToServer_f
*/
void CL_ForwardToServer_f( void )
{
	if( cls.state < CA_HANDSHAKE || cls.demo.playing || cls.state > CA_ACTIVE )
	{
		Com_Printf( "Can't \"%s\", not connected\n", Cmd_Argv( 0 ) );
		return;
	}

	// don't forward the first argument
	if( Cmd_Argc() > 1 )
		CL_AddReliableCommand( Cmd_Args() );
}

/*
* CL_ServerDisconnect_f
*/
void CL_ServerDisconnect_f( void )
{
	char menuparms[MAX_STRING_CHARS];
	int type;
	char reason[MAX_STRING_CHARS];

	type = atoi( Cmd_Argv( 1 ) );
	if( type < 0 || type >= DROP_TYPE_TOTAL )
		type = DROP_TYPE_GENERAL;

	Q_strncpyz( reason, Cmd_Argv( 2 ), sizeof( reason ) );

	CL_Disconnect_f();

	Com_Printf( "Connection was closed by server: %s\n", reason );

	Q_snprintfz( menuparms, sizeof( menuparms ), "menu_failed 1 \"%s\" %i \"%s\"", cls.servername, type, reason );

	Cbuf_ExecuteText( EXEC_NOW, menuparms );
}

/*
* CL_Quit
*/
void CL_Quit( void )
{
	CL_Disconnect( NULL );
	Com_Quit();
}

/*
* CL_Quit_f
*/
static void CL_Quit_f( void )
{
	CL_Quit();
}

/*
* CL_SendConnectPacket
*
* We have gotten a challenge from the server, so try and
* connect.
*/
static void CL_SendConnectPacket( void )
{
	cls.quakePort = Cvar_Value( "qport" );
	userinfo_modified = qfalse;

	Netchan_OutOfBandPrint( NS_CLIENT, cls.serveraddress, "connect %i %i %i \"%s\"\n",
		APP_PROTOCOL_VERSION, cls.quakePort, cls.challenge, Cvar_Userinfo() );
}

/*
* CL_CheckForResend
*
* Resend a connect message if the last one has timed out
*/
static void CL_CheckForResend( void )
{
	unsigned int curtime;

	if( cls.demo.playing )
		return;

	// if the local server is running and we aren't then connect
	if( cls.state == CA_DISCONNECTED && Com_ServerState() )
	{
		CL_SetClientState( CA_CONNECTING );
		if( cls.servername )
			Mem_ZoneFree( cls.servername );
		cls.servername = ZoneCopyString( "loopback" );
		NET_StringToAddress( "loopback", &cls.serveraddress );
		if( cls.serveraddress.port == 0 )
			cls.serveraddress.port = BigShort( PORT_SERVER );
	}
	// resend if we haven't gotten a reply yet
	else if( cls.state == CA_CONNECTING )
	{
		curtime = Sys_Milliseconds();
		if( curtime - cls.connect_time < 3000 )
			return;

		cls.connect_count++;
		cls.connect_time = curtime; // for retransmit requests

		Com_Printf( "Connecting to %s...\n", cls.servername );

		Netchan_OutOfBandPrint( NS_CLIENT, cls.serveraddress, "getchallenge\n" );
	}
}

static void CL_Connect( const char *servername, netadr_t *serveraddress )
{
	CL_Disconnect( NULL );

	NET_Config( qtrue );    // allow remote

	cls.serveraddress = *serveraddress;
	if( cls.serveraddress.port == 0 )
		cls.serveraddress.port = BigShort( PORT_SERVER );

	if( cls.servername )
		Mem_ZoneFree( cls.servername );
	cls.servername = ZoneCopyString( servername );

	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );
	CL_SetClientState( CA_CONNECTING );
	cls.connect_time = -99999; // CL_CheckForResend() will fire immediately
	cls.connect_count = 0;
	cls.rejected = qfalse;
	cls.lastPacketReceivedTime = cls.realtime; // reset the timeout limit
}

/*
* CL_Connect_f
*/
static void CL_Connect_f( void )
{
	netadr_t serveraddress; // save of address before calling CL_Disconnect
	char password[64];
	const char *extension;
	char *connectstring, *connectstring_base;
	const char *tmp, *scheme = APP_URI_SCHEME, *proto_scheme = APP_URI_PROTO_SCHEME;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "usage: connect <server>\n" );
		return;
	}

	connectstring_base = TempCopyString( Cmd_Argv( 1 ) );
	connectstring = connectstring_base;

	if( !Q_strnicmp( connectstring, proto_scheme, strlen( proto_scheme ) ) )
		connectstring += strlen( proto_scheme );
	else if( !Q_strnicmp( connectstring, scheme, strlen( scheme ) ) )
		connectstring += strlen( scheme );

	extension = COM_FileExtension( connectstring );
	if( extension && !Q_stricmp( COM_FileExtension( connectstring ), APP_DEMO_EXTENSION_STR ) )
	{
		char *temp;
		size_t temp_size;
		const char *http_scheme = "http://";

		if( !Q_strnicmp( connectstring, http_scheme, strlen( http_scheme ) ) )
			connectstring += strlen( http_scheme );

		temp_size = strlen( "demo " ) + strlen( http_scheme ) + strlen( connectstring ) + 1;
		temp = Mem_TempMalloc( temp_size );
		Q_snprintfz( temp, temp_size, "demo %s%s", http_scheme, connectstring );

		Cbuf_ExecuteText( EXEC_NOW, temp );

		Mem_TempFree( temp );
		Mem_TempFree( connectstring_base );
		return;
	}

	if ( ( tmp = Q_strrstr(connectstring, "@") ) != NULL ) {
		Q_strncpyz( password, connectstring, min(sizeof(password),( tmp - connectstring + 1)) );
		Cvar_Set( "password", password );
		connectstring = connectstring + (tmp - connectstring) + 1;
	}

	if ( ( tmp = Q_strrstr(connectstring, "/") ) != NULL ) {
		connectstring[tmp - connectstring] = '\0';
	}

	if( !NET_StringToAddress( connectstring, &serveraddress ) )
	{
		Com_Printf( "Bad server address\n" );
		Mem_TempFree( connectstring_base );
		return;
	}

	Mem_TempFree( connectstring_base );

	CL_Connect( connectstring, &serveraddress );
}


/*
* CL_Rcon_f
*
* Send the rest of the command line over as
* an unconnected command.
*/
static void CL_Rcon_f( void )
{
	char message[1024];
	int i;
	netadr_t to;

	if( cls.demo.playing )
		return;

	if( strlen( rcon_client_password->string ) == 0 )
	{
		Com_Printf( "You must set 'rcon_password' before issuing an rcon command.\n" );
		return;
	}

	// check for msg len abuse (thx to r1Q2)
	if( strlen( Cmd_Args() ) + strlen( rcon_client_password->string ) + 16 >= sizeof( message ) )
	{
		Com_Printf( "Length of password + command exceeds maximum allowed length.\n" );
		return;
	}

	message[0] = (qbyte)255;
	message[1] = (qbyte)255;
	message[2] = (qbyte)255;
	message[3] = (qbyte)255;
	message[4] = 0;

	NET_Config( qtrue );    // allow remote

	Q_strncatz( message, "rcon ", sizeof( message ) );

	Q_strncatz( message, rcon_client_password->string, sizeof( message ) );
	Q_strncatz( message, " ", sizeof( message ) );

	for( i = 1; i < Cmd_Argc(); i++ )
	{
		Q_strncatz( message, "\"", sizeof( message ) );
		Q_strncatz( message, Cmd_Argv( i ), sizeof( message ) );
		Q_strncatz( message, "\" ", sizeof( message ) );
	}

	if( cls.state >= CA_CONNECTED )
	{
		to = cls.netchan.remoteAddress;
	}
	else
	{
		if( !strlen( rcon_address->string ) )
		{
			Com_Printf( "You must be connected, or set the 'rcon_address' cvar to issue rcon commands\n" );
			return;
		}

		if( rcon_address->modified )
		{
			if( !NET_StringToAddress( rcon_address->string, &cls.rconaddress ) )
			{
				Com_Printf( "Bad rcon_address.\n" );
				return; // we don't clear modified, so it will whine the next time too
			}
			if( cls.rconaddress.port == 0 )
				cls.rconaddress.port = BigShort( PORT_SERVER );

			rcon_address->modified = qfalse;
		}

		to = cls.rconaddress;
	}

	NET_SendPacket( NS_CLIENT, (int)strlen( message )+1, message, to );
}

/*
* CL_GetClipboardData
*/
char *CL_GetClipboardData( qboolean primary )
{
	return Sys_GetClipboardData( primary );
}

/*
=====================
CL_SetClipboardData
=====================
*/
qboolean CL_SetClipboardData( char *data )
{
	return Sys_SetClipboardData( data );
}

/*
* CL_FreeClipboardData
*/
void CL_FreeClipboardData( char *data )
{
	Sys_FreeClipboardData( data );
}

/*
* CL_SetKeyDest
*/
void CL_SetKeyDest( int key_dest )
{
	if( key_dest < key_game || key_dest > key_delegate )
		Com_Error( ERR_DROP, "CL_SetKeyDest: invalid key_dest" );

	if( cls.key_dest != key_dest )
	{
		Key_ClearStates();
		cls.key_dest = key_dest;
	}
}


/*
* CL_SetOldKeyDest
*/
void CL_SetOldKeyDest( int key_dest )
{
	if( key_dest < key_game || key_dest > key_delegate )
		Com_Error( ERR_DROP, "CL_SetKeyDest: invalid key_dest" );
	cls.old_key_dest = key_dest;
}

/*
* CL_ResetServerCount
*/
void CL_ResetServerCount( void )
{
	cl.servercount = -1;
}

/*
* CL_ClearState
*/
void CL_ClearState( void )
{
	if( cl.cms )
	{
		CM_Free( cl.cms );
		cl.cms = NULL;
	}

	// wipe the entire cl structure
	memset( &cl, 0, sizeof( cl ) );

	//userinfo_modified = qtrue;
	cls.lastExecutedServerCommand = 0;
	cls.reliableAcknowledge = 0;
	cls.reliableSequence = 0;
	cls.reliableSent = 0;
	memset( cls.reliableCommands, 0, sizeof( cls.reliableCommands ) );

	// reset ucmds buffer
	cls.ucmdHead = 0;
	cls.ucmdSent = 0;
	cls.ucmdAcknowledged = 0;

	//restart realtime and lastPacket times
	cls.realtime = 0;
	cls.gametime = 0;
	cls.lastPacketSentTime = 0;
	cls.lastPacketReceivedTime = 0;

	cls.sv_pure = qfalse;
}


/*
* CL_SetNext_f
*
* Next is used to set an action which is executed at disconnecting.
*/
char cl_NextString[MAX_STRING_CHARS];
static void CL_SetNext_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Com_Printf( "USAGE: next <commands>\n" );
		return;
	}

	// jalfixme: I'm afraid of this being too powerful, since it basically
	// is allowed to execute everything. Shall we check for something?
	Q_strncpyz( cl_NextString, Cmd_Args(), sizeof( cl_NextString ) );
	Com_Printf( "NEXT: %s\n", cl_NextString );
}

/*
* CL_ExecuteNext
*/
static void CL_ExecuteNext( void )
{
	if( !strlen( cl_NextString ) )
		return;

	Cbuf_ExecuteText( EXEC_APPEND, cl_NextString );
	memset( cl_NextString, 0, sizeof( cl_NextString ) );
}

/*
* CL_Disconnect_SendCommand
*
* Sends a disconnect message to the server
*/
static void CL_Disconnect_SendCommand( void )
{
	// send the packet 3 times to make sure isn't lost
	CL_AddReliableCommand( "disconnect" );
	CL_SendMessagesToServer( qtrue );
	CL_AddReliableCommand( "disconnect" );
	CL_SendMessagesToServer( qtrue );
	CL_AddReliableCommand( "disconnect" );
	CL_SendMessagesToServer( qtrue );
}

/*
* CL_Disconnect
*
* Goes from a connected state to full screen console state
* Sends a disconnect message to the server
* This is also called on Com_Error, so it shouldn't cause any errors
*/
void CL_Disconnect( const char *message )
{
	char menuparms[MAX_STRING_CHARS];
	qboolean wasconnecting;

	// We have to shut down web-downloading first
	if( cls.download.web )
	{
		cls.download.disconnect = qtrue;
		return;
	}

	if( cls.state == CA_UNINITIALIZED )
		return;
	if( cls.state == CA_DISCONNECTED )
		goto done;

	if( cls.state < CA_LOADING )
		wasconnecting = qtrue;
	else
		wasconnecting = qfalse;

	if( cl_timedemo && cl_timedemo->integer )
	{
		unsigned int time;
		int i;

		Com_Printf( "\n" );
		for( i = 1; i < 100; i++ )
		{
			if( cl.timedemo.counts[i] > 0 )
			{
				Com_Printf( "%2ims - %7.2ffps: %6.2f%c\n", i, 1000.0/i,
					( cl.timedemo.counts[i]*1.0/cl.timedemo.frames )*100.0, '%' );
			}
		}

		Com_Printf( "\n" );
		time = Sys_Milliseconds() - cl.timedemo.start;
		if( time > 0 )
			Com_Printf( "%i frames, %3.1f seconds: %3.1f fps\n", cl.timedemo.frames,
			time/1000.0, cl.timedemo.frames*1000.0 / time );
	}

	cls.connect_time = 0;
	cls.connect_count = 0;
	cls.rejected = 0;

	if( cls.demo.recording )
		CL_Stop_f();

	if( SCR_GetCinematicTime() )
		SCR_StopCinematic();
	else if( cls.demo.playing )
		CL_DemoCompleted();
	else
		CL_Disconnect_SendCommand(); // send a disconnect message to the server

	FS_RemovePurePaks();
	Com_FreePureList( &cls.purelist );

	cls.sv_pure = qfalse;

	CL_RestartMedia();

	CL_ClearState();
	CL_SetClientState( CA_DISCONNECTED );

	if( cls.download.requestname )
	{
		if( cls.download.name )
		{
			assert( !cls.download.web );

			FS_RemoveBaseFile( cls.download.tempname );
			CL_StopServerDownload();
		}
		CL_DownloadDone();
	}

	if( message != NULL )
	{
		Q_snprintfz( menuparms, sizeof( menuparms ), "menu_failed %i \"%s\" %i \"%s\"", ( wasconnecting ? 0 : 2 ),
			cls.servername, DROP_TYPE_GENERAL, message );

		Cbuf_ExecuteText( EXEC_NOW, menuparms );
	}

done:
	// drop loading plaque unless this is the initial game start
	if( cls.disable_servercount != -1 )
		SCR_EndLoadingPlaque(); // get rid of loading plaque

	// in case we disconnect while in download phase
	CL_FreeDownloadList();

	CL_ExecuteNext(); // start next action if any is defined
}

void CL_Disconnect_f( void )
{
	// We have to shut down webdownloading first
	if( cls.download.web )
	{
		cls.download.disconnect = qtrue;
		return;
	}

	CL_Disconnect( NULL );
	SV_Shutdown( "Owner left the listen server", qfalse );
}

/*
* CL_Changing_f
*
* Just sent as a hint to the client that they should
* drop to full console
*/
void CL_Changing_f( void )
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if( cls.download.filenum || cls.download.web )
		return;

	Com_Printf( "CL:Changing\n" );

	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );
	CL_SetClientState( CA_CONNECTED ); // not active anymore, but not disconnected
}

/*
* CL_ServerReconnect_f
*
* The server is changing levels
*/
void CL_ServerReconnect_f( void )
{
	if( cls.demo.playing )
		return;

	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if( cls.download.filenum || cls.download.web )
	{
		cls.download.pending_reconnect = qtrue;
		return;
	}

	cls.connect_count = 0;
	cls.rejected = 0;

	CL_SoundModule_StopAllSounds();

	Com_Printf( "reconnecting...\n" );
	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );
	CL_SetClientState( CA_HANDSHAKE );
	CL_AddReliableCommand( "new" );
}

/*
* CL_Reconnect_f
*
* User reconnect command.
*/
void CL_Reconnect_f( void )
{
	CL_Disconnect( NULL );
	CL_ServerReconnect_f();
}

/*
* CL_ConnectionlessPacket
*
* Responses to broadcasts, etc
*/
static void CL_ConnectionlessPacket( msg_t *msg )
{
	char *s;
	char *c;

	MSG_BeginReading( msg );
	MSG_ReadLong( msg ); // skip the -1

	s = MSG_ReadStringLine( msg );

	if( !strncmp( s, "getserversResponse\\", 19 ) )
	{
		Com_Printf( "%s: %s\n", NET_AddressToString( &net_from ), "getserversResponse" );
		CL_ParseGetServersResponse( msg );
		return;
	}

	Cmd_TokenizeString( s );
	c = Cmd_Argv( 0 );

	Com_Printf( "%s: %s\n", NET_AddressToString( &net_from ), s );

	// server connection
	if( !strcmp( c, "client_connect" ) )
	{
		if( cls.state == CA_CONNECTED )
		{
			Com_Printf( "Dup connect received.  Ignored.\n" );
			return;
		}
		// these two are from Q3
		if( cls.state != CA_CONNECTING )
		{
			Com_Printf( "client_connect packet while not connecting.  Ignored.\n" );
			return;
		}
		if( !NET_CompareBaseAdr( &net_from, &cls.serveraddress ) )
		{
			Com_Printf( "client_connect from a different address.  Ignored.\n" );
			Com_Printf( "Was %s should have been %s\n", NET_AddressToString( &net_from ),
				NET_AddressToString( &cls.serveraddress ) );
			return;
		}

		cls.rejected = qfalse;

		Netchan_Setup( NS_CLIENT, &cls.netchan, net_from, cls.quakePort );
		memset( cl.configstrings, 0, sizeof( cl.configstrings ) );
		CL_SetClientState( CA_HANDSHAKE );
		CL_AddReliableCommand( "new" );
		return;
	}

	// reject packet, used to inform the client that connection attemp didn't succeed
	if( !strcmp( c, "reject" ) )
	{
		int rejectflag;

		if( cls.state != CA_CONNECTING )
		{
			Com_Printf( "reject packet while not connecting.  Ignored.\n" );
			return;
		}
		if( !NET_CompareBaseAdr( &net_from, &cls.serveraddress ) )
		{
			Com_Printf( "reject from a different address.  Ignored.\n" );
			Com_Printf( "Was %s should have been %s\n", NET_AddressToString( &net_from ),
				NET_AddressToString( &cls.serveraddress ) );
			return;
		}

		cls.rejected = qtrue;

		cls.rejecttype = atoi( MSG_ReadStringLine( msg ) );
		if( cls.rejecttype < 0 || cls.rejecttype >= DROP_TYPE_TOTAL ) cls.rejecttype = DROP_TYPE_GENERAL;

		rejectflag = atoi( MSG_ReadStringLine( msg ) );

		Q_strncpyz( cls.rejectmessage, MSG_ReadStringLine( msg ), sizeof( cls.rejectmessage ) );
		if( strlen( cls.rejectmessage ) > sizeof( cls.rejectmessage )-2 )
		{
			cls.rejectmessage[strlen( cls.rejectmessage )-2] = '.';
			cls.rejectmessage[strlen( cls.rejectmessage )-1] = '.';
			cls.rejectmessage[strlen( cls.rejectmessage )] = '.';
		}

		Com_Printf( "Connection refused: %s\n", cls.rejectmessage );
		if( rejectflag & DROP_FLAG_AUTORECONNECT )
		{
			Com_Printf( "Automatic reconnecting allowed.\n" );
		}
		else
		{
			char menuparms[MAX_STRING_CHARS];

			Com_Printf( "Automatic reconnecting not allowed.\n" );

			CL_Disconnect( NULL );
			Q_snprintfz( menuparms, sizeof( menuparms ), "menu_failed 0 \"%s\" %i \"%s\"",
				cls.servername, cls.rejecttype, cls.rejectmessage );

			Cbuf_ExecuteText( EXEC_NOW, menuparms );
		}

		return;
	}

	// server responding to a status broadcast
	if( !strcmp( c, "info" ) )
	{
		CL_ParseStatusMessage( msg );
		return;
	}

	// remote command from gui front end
	if( !strcmp( c, "cmd" ) )
	{
		if( !NET_IsLocalAddress( &net_from ) )
		{
			Com_Printf( "Command packet from remote host.  Ignored.\n" );
			return;
		}
		Sys_AppActivate();
		s = MSG_ReadString( msg );
		Cbuf_AddText( s );
		Cbuf_AddText( "\n" );
		return;
	}
	// print command from somewhere
	if( !strcmp( c, "print" ) )
	{
		// CA_CONNECTING is allowed, because old servers send protocol mismatch connection error message with it
		if( ( ( cls.state != CA_UNINITIALIZED && cls.state != CA_DISCONNECTED ) &&
			NET_CompareBaseAdr( &net_from, &cls.serveraddress ) ) ||
			( strlen( rcon_address->string ) > 0 && NET_CompareBaseAdr( &net_from, &cls.rconaddress ) ) )
		{
			s = MSG_ReadString( msg );
			Com_Printf( "%s", s );
			return;
		}
		else
		{
			Com_Printf( "Print packet from unknown host.  Ignored.\n" );
			return;
		}
	}

	// ping from somewhere
	if( !strcmp( c, "ping" ) )
	{
		Netchan_OutOfBandPrint( NS_CLIENT, net_from, "ack" );
		return;
	}

	// challenge from the server we are connecting to
	if( !strcmp( c, "challenge" ) )
	{
		// these two are from Q3
		if( cls.state != CA_CONNECTING )
		{
			Com_Printf( "challenge packet while not connecting.  Ignored.\n" );
			return;
		}
		if( !NET_CompareBaseAdr( &net_from, &cls.serveraddress ) )
		{
			Com_Printf( "challenge from a different address.  Ignored.\n" );
			Com_Printf( "Was %s ", NET_AddressToString( &net_from ) );
			Com_Printf( "should have been %s\n", NET_AddressToString( &cls.serveraddress ) );
			return;
		}

		cls.challenge = atoi( Cmd_Argv( 1 ) );
		// r1: reset the timer so we don't send dup. getchallenges
		cls.connect_time = Sys_Milliseconds();
		CL_SendConnectPacket();
		return;
	}

	// echo request from server
	if( !strcmp( c, "echo" ) )
	{
		Netchan_OutOfBandPrint( NS_CLIENT, net_from, "%s", Cmd_Argv( 1 ) );
		return;
	}

	// server responding to a detailed info broadcast
	if( !strcmp( c, "infoResponse" ) )
	{
		CL_ParseGetInfoResponse( msg );
		return;
	}

	if( !strcmp( c, "statusResponse" ) )
	{
		CL_ParseGetStatusResponse( msg );
		return;
	}

	Com_Printf( "Unknown command.\n" );
}

/*
* CL_ProcessPacket
*/
static qboolean CL_ProcessPacket( netchan_t *netchan, msg_t *msg )
{
	int sequence, sequence_ack;
	int zerror;

	if( !Netchan_Process( netchan, msg ) )
		return qfalse; // wasn't accepted for some reason

	// now if compressed, expand it
	MSG_BeginReading( msg );
	sequence = MSG_ReadLong( msg );
	sequence_ack = MSG_ReadLong( msg );
	if( msg->compressed )
	{
		zerror = Netchan_DecompressMessage( msg );
		if( zerror < 0 )
		{          // compression error. Drop the packet
			Com_DPrintf( "CL_ProcessPacket: Compression error %i. Dropping packet\n", zerror );
			return qfalse;
		}
	}

	return qtrue;
}

/*
* CL_ReadPackets
*/
void CL_ReadPackets( void )
{
	static msg_t msg;
	static qbyte msgData[MAX_MSGLEN];

	MSG_Init( &msg, msgData, sizeof( msgData ) );
	MSG_Clear( &msg );

	while( NET_GetPacket( NS_CLIENT, &net_from, &msg ) )
	{
		//
		// remote command packet
		//
		if( *(int *)msg.data == -1 )
		{
			CL_ConnectionlessPacket( &msg );
			continue;
		}

		if( cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING )
			continue; // dump it if not connected

		if( msg.cursize < 8 )
		{
			Com_DPrintf( "%s: Runt packet\n", NET_AddressToString( &net_from ) );
			continue;
		}

		//
		// packet from server
		//
		if( !NET_CompareAdr( &net_from, &cls.netchan.remoteAddress ) )
		{
			Com_DPrintf( "%s:sequenced packet without connection\n"
				, NET_AddressToString( &net_from ) );
			continue;
		}
		if( !CL_ProcessPacket( &cls.netchan, &msg ) )
			continue; // wasn't accepted for some reason

		CL_ParseServerMessage( &msg );
		cls.lastPacketReceivedTime = cls.realtime;
	}

	// not expected, but could happen if cls.realtime is cleared and lastPacketReceivedTime is not
	if( cls.lastPacketReceivedTime > cls.realtime )
		cls.lastPacketReceivedTime = cls.realtime;

	// check timeout
	if( cls.state >= CA_HANDSHAKE && !SCR_GetCinematicTime() && cls.lastPacketReceivedTime )
	{
		if( cls.lastPacketReceivedTime + cl_timeout->value*1000 < cls.realtime )
		{
			if( ++cl.timeoutcount > 5 ) // timeoutcount saves debugger
			{
				Com_Printf( "\nServer connection timed out.\n" );
				CL_Disconnect( "Connection timed out" );
				return;
			}
		}
	}
	else
		cl.timeoutcount = 0;

}

//=============================================================================

/*
* CL_Userinfo_f
*/
static void CL_Userinfo_f( void )
{
	Com_Printf( "User info settings:\n" );
	Info_Print( Cvar_Userinfo() );
}

static unsigned int CL_LoadMap( const char *name )
{
	unsigned int map_checksum;

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	assert( !cl.cms );
	cl.cms = CM_New( NULL );
	CM_LoadMap( cl.cms, name, qtrue, &map_checksum );

	assert( cl.cms );

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	return map_checksum;
}

static int precache_check; // for autodownload of precache items
static int precache_spawncount;
static int precache_tex;
static int precache_pure;

#define PLAYER_MULT 5

// ENV_CNT is map load
#define ENV_CNT ( CS_SKINFILES + MAX_SKINFILES )
#define TEXTURE_CNT ( ENV_CNT+1 )

void CL_RequestNextDownload( void )
{
	char tempname[MAX_CONFIGSTRING_CHARS + 4];
	purelist_t *purefile;
	int i;

	if( cls.state != CA_CONNECTED )
		return;

	// pure list
	if( cls.sv_pure )
	{
		// skip
		if( !cl_downloads->integer )
			precache_pure = -1;

		// try downloading
		if( precache_pure != -1 )
		{
			i = 0;
			purefile = cls.purelist;
			while( i < precache_pure && purefile )
			{
				purefile = purefile->next;
				i++;
			}

			while( purefile )
			{
				precache_pure++;
				if( !CL_CheckOrDownloadFile( purefile->filename ) )
					return;
				purefile = purefile->next;
			}
			precache_pure = -1;
		}

		if( precache_pure == -1 )
		{
			qboolean failed = qfalse;
			char message[MAX_STRING_CHARS];

			Q_snprintfz( message, sizeof( message ), "Pure check failed:" );

			purefile = cls.purelist;
			while( purefile )
			{
				Com_DPrintf( "Adding pure file: %s\n", purefile->filename );
				if( !FS_AddPurePak( purefile->checksum ) )
				{
					failed = qtrue;
					Q_strncatz( message, " ", sizeof( message ) );
					Q_strncatz( message, purefile->filename, sizeof( message ) );
				}
				purefile = purefile->next;
			}

			if( failed )
			{
				Com_Error( ERR_DROP, message );
				return;
			}
		}
	}

	// skip if download not allowed
	if( !cl_downloads->integer && precache_check < ENV_CNT )
		precache_check = ENV_CNT;

	if( precache_check == CS_WORLDMODEL ) // confirm map
	{
		precache_check = CS_MODELS;

		if( !CL_CheckOrDownloadFile( cl.configstrings[CS_WORLDMODEL] ) )
			return; // started a download
	}

	if( precache_check >= CS_MODELS && precache_check < CS_MODELS+MAX_MODELS )
	{
		while( precache_check < CS_MODELS+MAX_MODELS && cl.configstrings[precache_check][0] )
		{
			if( cl.configstrings[precache_check][0] == '*' ||
				cl.configstrings[precache_check][0] == '$' || // disable player model downloading for now
				cl.configstrings[precache_check][0] == '#' )
			{
				precache_check++;
				continue;
			}

			if( !CL_CheckOrDownloadFile( cl.configstrings[precache_check++] ) )
				return; // started a download
		}
		precache_check = CS_SOUNDS;
	}

	if( precache_check >= CS_SOUNDS && precache_check < CS_SOUNDS+MAX_SOUNDS )
	{
		if( precache_check == CS_SOUNDS )
			precache_check++; // zero is blank

		while( precache_check < CS_SOUNDS+MAX_SOUNDS && cl.configstrings[precache_check][0] )
		{
			if( cl.configstrings[precache_check][0] == '*' ) // sexed sounds
			{
				precache_check++;
				continue;
			}
			Q_strncpyz( tempname, cl.configstrings[precache_check++], sizeof( tempname ) );
			if( !COM_FileExtension( tempname ) )
			{
				if( !FS_FirstExtension( tempname, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS ) )
				{
					COM_DefaultExtension( tempname, ".wav", sizeof( tempname ) );
					if( !CL_CheckOrDownloadFile( tempname ) )
						return; // started a download
				}
			}
			else
			{
				if( !CL_CheckOrDownloadFile( tempname ) )
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}
	if( precache_check >= CS_IMAGES && precache_check < CS_IMAGES+MAX_IMAGES )
	{
		if( precache_check == CS_IMAGES )
			precache_check++; // zero is blank

		// precache phase completed
		precache_check = ENV_CNT;
	}

	if( precache_check == ENV_CNT )
	{
		unsigned map_checksum;

		// we're done with the download phase, so clear the list
		CL_FreeDownloadList();
		if( cls.sv_pure )
		{
			Com_Printf( "Pure server. Restarting media...\n" );
			CL_RestartMedia();
		}
		else if( cls.download.successCount )
		{
			Com_Printf( "Files downloaded. Restarting media...\n" );
			CL_RestartMedia();
		}
		cls.download.successCount = 0;

		map_checksum = CL_LoadMap( cl.configstrings[CS_WORLDMODEL] );
		if( map_checksum != (unsigned)atoi( cl.configstrings[CS_MAPCHECKSUM] ) )
		{
			Com_Error( ERR_DROP, "Local map version differs from server: %u != '%u'",
				map_checksum, (unsigned)atoi(cl.configstrings[CS_MAPCHECKSUM]) );
			return;
		}

		precache_check = TEXTURE_CNT;
	}

	if( precache_check == TEXTURE_CNT )
	{
		precache_check = TEXTURE_CNT+1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if( precache_check == TEXTURE_CNT+1 )
		precache_check = TEXTURE_CNT+999;

	// load client game module
	CL_GameModule_Init();
	CL_AddReliableCommand( va( "begin %i\n", precache_spawncount ) );
}

/*
* CL_Precache_f
*
* The server will send this command right
* before allowing the client into the server
*/
void CL_Precache_f( void )
{
	FS_RemovePurePaks();

	if( Cmd_Argc() < 2 )
	{ 
		// demo playback
		if( !cls.demo.playing || !cls.demo.play_jump )
		{
			CL_LoadMap( cl.configstrings[CS_WORLDMODEL] );
		}

		CL_GameModule_Init();

		if( cls.demo.playing )
			cls.demo.play_ignore_next_frametime = qtrue;

		return;
	}

	precache_pure = 0;
	precache_check = CS_WORLDMODEL;
	precache_spawncount = atoi( Cmd_Argv( 1 ) );

	CL_RequestNextDownload();
}

/*
* CL_WriteConfiguration
*
* Writes key bindings, archived cvars and aliases to a config file
*/
static void CL_WriteConfiguration( const char *name, qboolean warn )
{
	int file;

	if( FS_FOpenFile( name, &file, FS_WRITE ) == -1 )
	{
		Com_Printf( "Couldn't write %s.\n", name );
		return;
	}

	if( warn )
		FS_Printf( file, "// This file is automatically generated by " APPLICATION ", do not modify.\n\n" );

	FS_Printf( file, "// key bindings\n" );
	Key_WriteBindings( file );

	FS_Printf( file, "\n// variables\n" );
	Cvar_WriteVariables( file );

	FS_Printf( file, "\n// aliases\n" );
	Cmd_WriteAliases( file );

	FS_FCloseFile( file );
}


/*
* CL_WriteConfig_f
*/
static void CL_WriteConfig_f( void )
{
	char *name;
	int name_size;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	name_size = sizeof( char ) * ( strlen( Cmd_Argv( 1 ) ) + strlen( ".cfg" ) + 1 );
	name = Mem_TempMalloc( name_size );
	Q_strncpyz( name, Cmd_Argv( 1 ), name_size );
	COM_SanitizeFilePath( name );

	if( !COM_ValidateRelativeFilename( name ) )
	{
		Com_Printf( "Invalid filename" );
		Mem_TempFree( name );
		return;
	}

	COM_DefaultExtension( name, ".cfg", name_size );

	Com_Printf( "Writing: %s\n", name );
	CL_WriteConfiguration( name, qfalse );

	Mem_TempFree( name );
}

/*
* CL_SetClientState
*/
void CL_SetClientState( int state )
{
	cls.state = state;
	Com_SetClientState( state );

	switch( state )
	{
	case CA_DISCONNECTED:
		Con_Close();
		Cbuf_ExecuteText( EXEC_NOW, "menu_main" );
		CL_SetKeyDest( key_menu );
		// SCR_UpdateScreen();
		break;
	case CA_CONNECTING:
		Con_Close();
		CL_SetKeyDest( key_game );
		// SCR_UpdateScreen();
		break;
	case CA_HANDSHAKE:
		break;
	case CA_CONNECTED:
		Con_Close();
		CL_UIModule_DeactivateUI();
		Cvar_FixCheatVars();
		// SCR_UpdateScreen();
		break;
	case CA_LOADING:
		//Con_Close();
		break;
	case CA_ACTIVE:
		Con_Close();
		CL_SetKeyDest( key_game );
		// SCR_UpdateScreen();
		CL_SoundModule_Clear();
		break;
	default:
		break;
	}
}

/*
* CL_GetClientState
*/
connstate_t CL_GetClientState( void )
{
	return cls.state;
}

/*
* CL_InitMedia
*/
void CL_InitMedia( void )
{
	if( cls.mediaInitialized )
		return;
	if( cls.state == CA_UNINITIALIZED )
		return;

	cls.mediaInitialized = qtrue;

	// restart renderer
	R_Restart();

	// register console font and background
	SCR_RegisterConsoleMedia();

	// load user interface
	CL_UIModule_Init();

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	CL_SoundModule_Init( qfalse );
}

/*
* CL_ShutdownMedia
*/
void CL_ShutdownMedia( void )
{
	if( !cls.mediaInitialized )
		return;

	cls.mediaInitialized = qfalse;

	// shutdown cgame
	CL_GameModule_Shutdown();

	// shutdown user interface
	CL_UIModule_Shutdown();

	// stop and free all sounds
	CL_SoundModule_Shutdown( qfalse );

	// clear fonts etc
	SCR_ShutDownConsoleMedia();
}

/*
* CL_RestartMedia
*/
void CL_RestartMedia( void )
{
	CL_ShutdownMedia();
	CL_InitMedia();
}

/*
* CL_S_Restart
*
* Restart the sound subsystem so it can pick up new parameters and flush all sounds
*/
void CL_S_Restart( qboolean noVideo )
{
	qboolean verbose = ( Cmd_Argc() >= 2 ? qtrue : qfalse );

	/*
	Restart the sound subsystem
	The cgame and game must also be forced to restart because
	handles will be invalid
	VID_Restart forces an audio restart
	*/
	if( !noVideo )
	{
		VID_Restart( verbose );
		VID_CheckChanges();
	}
	else
	{
		CL_SoundModule_Shutdown( verbose );
		CL_SoundModule_Init( verbose );
	}
}
/*
* CL_S_Restart_f
*
* Restart the sound subsystem so it can pick up new parameters and flush all sounds
*/
static void CL_S_Restart_f( void )
{
	CL_S_Restart( qfalse );
}

/*
* CL_ShowIP_f - it only shows the ip when server was started
*/
static void CL_ShowIP_f( void )
{
	Sys_ShowIP();
}

/*
* CL_ShowServerIP_f - shows the ip:port of the server the client is connected to
*/
static void CL_ShowServerIP_f( void )
{

	if( cls.state < CA_CONNECTED || cls.state > CA_ACTIVE )
	{
		Com_Printf( "Not connected to a server\n" );
		return;
	}

	Com_Printf( "Connected to server:\n" );
	Com_Printf( "Name: %s\n", cls.servername );
	Com_Printf( "Address: %s\n", NET_AddressToString( &cls.serveraddress ) );
}

/*
* CL_InitLocal
*/
static void CL_InitLocal( void )
{
	cvar_t *color;

	cls.state = CA_DISCONNECTED;
	Com_SetClientState( CA_DISCONNECTED );

	//
	// register our variables
	//
	cl_stereo_separation =	Cvar_Get( "cl_stereo_separation", "0.4", CVAR_ARCHIVE );
	cl_stereo =		Cvar_Get( "cl_stereo", "0", CVAR_ARCHIVE );

	cl_maxfps =		Cvar_Get( "cl_maxfps", "90", CVAR_ARCHIVE );
#ifdef PUTCPU2SLEEP
	cl_sleep =		Cvar_Get( "cl_sleep", "0", CVAR_ARCHIVE );
#endif
	cl_extrapolate =	Cvar_Get( "cl_extrapolate", "1", CVAR_DEVELOPER );
	cl_pps =		Cvar_Get( "cl_pps", "63", CVAR_ARCHIVE );
	cl_compresspackets =	Cvar_Get( "cl_compresspackets", "1", CVAR_ARCHIVE );

	cl_yawspeed =		Cvar_Get( "cl_yawspeed", "140", 0 );
	cl_pitchspeed =		Cvar_Get( "cl_pitchspeed", "150", 0 );
	cl_anglespeedkey =	Cvar_Get( "cl_anglespeedkey", "1.5", 0 );

	m_pitch =		Cvar_Get( "m_pitch", "0.022", CVAR_ARCHIVE );
	m_yaw =			Cvar_Get( "m_yaw", "0.022", CVAR_ARCHIVE );

	cl_masterservers =	Cvar_Get( "masterservers", DEFAULT_MASTER_SERVERS_IPS, 0 );

	cl_shownet =		Cvar_Get( "cl_shownet", "0", 0 );
	cl_timeout =		Cvar_Get( "cl_timeout", "120", 0 );
	cl_timedemo =		Cvar_Get( "timedemo", "0", CVAR_CHEAT );
	cl_demoavi_video =	Cvar_Get( "cl_demoavi_video", "1", CVAR_ARCHIVE );
	cl_demoavi_audio =	Cvar_Get( "cl_demoavi_audio", "0", CVAR_ARCHIVE );
	cl_demoavi_fps =	Cvar_Get( "cl_demoavi_fps", "30.303030", CVAR_ARCHIVE );
	cl_demoavi_fps->modified = qtrue;
	cl_demoavi_scissor =	Cvar_Get( "cl_demoavi_scissor", "0", CVAR_ARCHIVE );

	rcon_client_password =	Cvar_Get( "rcon_password", "", 0 );
	rcon_address =		Cvar_Get( "rcon_address", "", 0 );

	cl_debug_serverCmd =	Cvar_Get( "cl_debug_serverCmd", "0", CVAR_DEVELOPER );
	cl_debug_timeDelta =	Cvar_Get( "cl_debug_timeDelta", "0", CVAR_DEVELOPER );

	cl_downloads =		Cvar_Get( "cl_downloads", "1", CVAR_ARCHIVE );
	cl_downloads_from_web =	Cvar_Get( "cl_downloads_from_web", "1", CVAR_ARCHIVE );
	cl_downloads_from_web_timeout = Cvar_Get( "cl_downloads_from_web_timeout", "600", CVAR_ARCHIVE );
	cl_download_allow_modules = Cvar_Get( "cl_download_allow_modules", "1", CVAR_ARCHIVE );

	//
	// userinfo
	//
	info_password =		Cvar_Get( "password", "", CVAR_USERINFO );

	Cvar_Get( "name", "player", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "model", "", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "skin", "", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "fov", "90", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "zoomfov", "40", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "zoomsens", "1.5", CVAR_ARCHIVE );

	Cvar_Get( "cl_download_name", "", CVAR_READONLY );
	Cvar_Get( "cl_download_percent", "0", CVAR_READONLY );

	color = Cvar_Get( "color", "", CVAR_ARCHIVE|CVAR_USERINFO );
	if( COM_ReadColorRGBString( color->string ) == -1 )
	{                                                 // first time create a random color for it
		time_t long_time; // random isn't working fine at this point.
		struct tm *newtime; // so we get the user local time and use some values from there
		time( &long_time );
		newtime = localtime( &long_time );
		long_time *= newtime->tm_sec * newtime->tm_min * newtime->tm_wday;
		Cvar_Set( "color", va( "%i %i %i", ( long_time )&0xff, ( ( long_time )>>8 )&0xff, ( ( long_time )>>16 )&0xff ) );
	}

	//
	// register our commands
	//
	Cmd_AddCommand( "s_restart", CL_S_Restart_f );
	Cmd_AddCommand( "cmd", CL_ForwardToServer_f );
	Cmd_AddCommand( "requestservers", CL_GetServers_f );
	Cmd_AddCommand( "getinfo", CL_QueryGetInfoMessage_f );
	Cmd_AddCommand( "getstatus", CL_QueryGetStatusMessage_f );
	Cmd_AddCommand( "userinfo", CL_Userinfo_f );
	Cmd_AddCommand( "disconnect", CL_Disconnect_f );
	Cmd_AddCommand( "record", CL_Record_f );
	Cmd_AddCommand( "stop", CL_Stop_f );
	Cmd_AddCommand( "quit", CL_Quit_f );
	Cmd_AddCommand( "connect", CL_Connect_f );
	Cmd_AddCommand( "reconnect", CL_Reconnect_f );
	Cmd_AddCommand( "rcon", CL_Rcon_f );
	Cmd_AddCommand( "writeconfig", CL_WriteConfig_f );
	Cmd_AddCommand( "showip", CL_ShowIP_f );
	Cmd_AddCommand( "demo", CL_PlayDemo_f );
	Cmd_AddCommand( "demoavi", CL_PlayDemoToAvi_f );
	Cmd_AddCommand( "cinematic", CL_PlayCinematic_f );
	Cmd_AddCommand( "next", CL_SetNext_f );
	Cmd_AddCommand( "pingserver", CL_PingServer_f );
	Cmd_AddCommand( "demojump", CL_DemoJump_f );
	Cmd_AddCommand( "showserverip", CL_ShowServerIP_f );
	Cmd_AddCommand( "downloadstatus", CL_DownloadStatus_f );
	Cmd_AddCommand( "downloadcancel", CL_DownloadCancel_f );
}

//============================================================================


#ifdef _DEBUG
static void CL_LogStats( void )
{
	static unsigned int lasttimecalled = 0;
	if( log_stats->integer )
	{
		if( cls.state == CA_ACTIVE )
		{
			if( !lasttimecalled )
			{
				lasttimecalled = Sys_Milliseconds();
				if( log_stats_file )
					FS_Printf( log_stats_file, "0\n" );
			}
			else
			{
				unsigned int now = Sys_Milliseconds();

				if( log_stats_file )
					FS_Printf( log_stats_file, "%u\n", now - lasttimecalled );
				lasttimecalled = now;
			}
		}
	}
}
#endif

/*
* CL_TimedemoStats
*/
static void CL_TimedemoStats( void )
{
	if( cl_timedemo->integer )
	{
		static int lasttime = 0;
		int curtime = Sys_Milliseconds();
		if( lasttime != 0 )
		{
			if( curtime - lasttime >= 100 )
				cl.timedemo.counts[99]++;
			else
				cl.timedemo.counts[curtime-lasttime]++;
		}
		lasttime = curtime;
	}
}

/*
* CL_Netchan_Transmit
*/
void CL_Netchan_Transmit( msg_t *msg )
{
	int zerror;

	// if we got here with unsent fragments, fire them all now
	Netchan_PushAllFragments( &cls.netchan );

	if( ( cl_compresspackets->integer && msg->cursize > 80 ) || cl_compresspackets->integer > 1 )
	{
		zerror = Netchan_CompressMessage( msg );
		if( zerror < 0 ) // it's compression error, just send uncompressed
		{
			Com_DPrintf( "CL_Netchan_Transmit (ignoring compression): Compression error %i\n", zerror );
		}
	}

	Netchan_Transmit( &cls.netchan, msg );
	cls.lastPacketSentTime = cls.realtime;
}

/*
* CL_MaxPacketsReached
*/
static qboolean CL_MaxPacketsReached( void )
{
	static unsigned int lastPacketTime = 0;
	static float roundingMsec = 0.0f;
	int minpackettime;
	int elapsedTime;
	float minTime;

	if( lastPacketTime > cls.realtime )
		lastPacketTime = cls.realtime;

	if( cl_pps->integer > 63 || cl_pps->integer < 20 )
	{
		Com_Printf( "'cl_pps' value is out of valid range, resetting to default\n" );
		Cvar_ForceSet( "cl_pps", va( "%s", cl_pps->dvalue ) );
	}

	elapsedTime = cls.realtime - lastPacketTime;
	minTime = ( 1000.0f / cl_pps->value );

	// don't let cl_pps be smaller than sv_pps
	if( cls.state == CA_ACTIVE && !cls.demo.playing && cl.snapFrameTime )
	{
		if( (unsigned int)minTime > cl.snapFrameTime )
			minTime = cl.snapFrameTime;
	}

	minpackettime = (int)minTime;
	roundingMsec += minTime - (int)minTime;
	if( roundingMsec >= 1.0f )
	{
		minpackettime += (int)roundingMsec;
		roundingMsec -= (int)roundingMsec;
	}

	if( elapsedTime < minpackettime )
		return qfalse;

	lastPacketTime = cls.realtime;
	return qtrue;
}

/*
* CL_SendMessagesToServer
*/
void CL_SendMessagesToServer( qboolean sendNow )
{
	msg_t message;
	qbyte messageData[MAX_MSGLEN];

	if( cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING )
		return;

	if( cls.demo.playing )
		return;

	if( SCR_GetCinematicTime() )
		return;

	MSG_Init( &message, messageData, sizeof( messageData ) );
	MSG_Clear( &message );

	// write the command ack
	MSG_WriteByte( &message, clc_svcack );
	MSG_WriteLong( &message, (unsigned int)cls.lastExecutedServerCommand );

	// send only reliable commands during conneting time
	if( sendNow || ( cls.state < CA_ACTIVE && cls.realtime > cls.lastPacketSentTime + 1000 ) )
	{
		// write the clc commands
		CL_UpdateClientCommandsToServer( &message );
		CL_Netchan_Transmit( &message );
		return;

	}
	else if( CL_MaxPacketsReached() )
	{
		// send a userinfo update if needed
		if( userinfo_modified )
		{
			userinfo_modified = qfalse;
			CL_AddReliableCommand( va( "usri \"%s\"", Cvar_Userinfo() ) );
		}

		CL_UpdateClientCommandsToServer( &message );
		CL_WriteUcmdsToMessage( &message );
		CL_Netchan_Transmit( &message );
	}
}

/*
* CL_NetFrame
*/
static void CL_NetFrame( int realmsec )
{
	// read packets from server
	if( realmsec > 5000 )  // if in the debugger last frame, don't timeout
		cls.lastPacketReceivedTime = cls.realtime;

	if( cls.demo.playing )
		CL_ReadDemoPackets(); // fetch results from demo file
	else
		CL_ReadPackets(); // fetch results from server

	// send packets to server
	if( cls.netchan.unsentFragments )
		Netchan_TransmitNextFragment( &cls.netchan );
	else
		CL_SendMessagesToServer( qfalse );

	// resend a connection request if necessary
	CL_CheckForResend();
	CL_CheckDownloadTimeout();
}

/*
* CL_AdjustServerTime - adjust delta to new frame snap timestamp
*/
void CL_AdjustServerTime( unsigned int gamemsec )
{
	// hurry up if coming late
	if( ( cl.newServerTimeDelta < cl.serverTimeDelta ) && ( gamemsec > 1 ) )
		cl.serverTimeDelta--;
	if( cl.newServerTimeDelta > cl.serverTimeDelta )
		cl.serverTimeDelta++;

	if( cls.gametime + cl.serverTimeDelta < 0 )  // should never happen
		cl.serverTimeDelta = 0;

	cl.serverTime = cls.gametime + cl.serverTimeDelta;

	// it launches a new snapshot when the timestamp of the CURRENT snap is reached.
	if( cl.pendingSnapNum && ( cl.serverTime >= cl.snapShots[cl.currentSnapNum & SNAPS_BACKUP_MASK].timeStamp ) )
	{
		// fire next snapshot
		if( CL_GameModule_NewSnapshot( cl.pendingSnapNum, cl.serverTime ) )
		{
			cl.previousSnapNum = cl.currentSnapNum;
			cl.currentSnapNum = cl.pendingSnapNum;
			cl.pendingSnapNum = 0;

			// getting a valid snapshot ends the connection process
			if( cls.state == CA_CONNECTED )
				CL_SetClientState( CA_ACTIVE );
		}
	}
}

/*
* CL_RestartTimeDeltas
*/
void CL_RestartTimeDeltas( unsigned int newTimeDelta )
{
	int i;

	cl.serverTimeDelta = cl.newServerTimeDelta = newTimeDelta;
	for( i = 0; i < MAX_TIMEDELTAS_BACKUP; i++ )
		cl.serverTimeDeltas[i] = newTimeDelta;

	if( cl_debug_timeDelta->integer )
		Com_Printf( S_COLOR_CYAN"***** timeDelta restarted\n" );
}

/*
* CL_SmoothTimeDeltas
*/
int CL_SmoothTimeDeltas( void )
{
	unsigned int i;
	int count;
	double delta;
	snapshot_t *snap;

	if( cls.demo.playing )
	{
		if( cl.currentSnapNum <= 0 ) // if first snap
			return cl.serverTimeDeltas[cl.pendingSnapNum & MASK_TIMEDELTAS_BACKUP];

		return cl.serverTimeDeltas[cl.currentSnapNum & MASK_TIMEDELTAS_BACKUP];
	}

	count = cl.receivedSnapNum - MASK_TIMEDELTAS_BACKUP;
	if( count < 0 )
		count = 0;
	i = count;

	for( delta = 0, count = 0; i <= cl.receivedSnapNum; i++ )
	{
		snap = &cl.snapShots[i & SNAPS_BACKUP_MASK];
		if( snap->valid && snap->snapNum == i )
		{
			delta += (double)cl.serverTimeDeltas[i & MASK_TIMEDELTAS_BACKUP];
			count++;
		}
	}

	if( !count )
		return 0;

	return (int)( delta / (double)count );
}

/*
* CL_UpdateSnapshot - Check for pending snapshots, and fire if needed
*/
void CL_UpdateSnapshot( void )
{
	snapshot_t *snap;
	unsigned int i;

	// see if there is any pending snap to be fired
	if( !cl.pendingSnapNum && ( cl.currentSnapNum != cl.receivedSnapNum ) )
	{
		snap = NULL;
		for( i = cl.currentSnapNum + 1; i <= cl.receivedSnapNum; i++ )
		{
			if( cl.snapShots[i & SNAPS_BACKUP_MASK].valid && ( cl.snapShots[i & SNAPS_BACKUP_MASK].snapNum > cl.currentSnapNum ) )
			{
				snap = &cl.snapShots[i & SNAPS_BACKUP_MASK];
			}
		}

		if( snap ) // valid pending snap found
		{
			cl.pendingSnapNum = snap->snapNum;

			cl.newServerTimeDelta = CL_SmoothTimeDeltas() + cl.extrapolationTime;

			// if we don't have current snap (or delay is too big) don't wait to fire the pending one
			if( cl.currentSnapNum <= 0 || ( abs( cl.newServerTimeDelta - cl.serverTimeDelta ) > 200 ) )
			{
				cl.serverTimeDelta = cl.newServerTimeDelta;
			}

			// don't either wait if in a timedemo
			if( cls.demo.playing && cl_timedemo->integer )
			{
				cl.serverTimeDelta = cl.newServerTimeDelta;
			}
		}
	}
}

/*
* CL_Frame
*/
void CL_Frame( int realmsec, int gamemsec )
{
	static int allRealMsec = 0, allGameMsec = 0, extraMsec = 0;
	static float roundingMsec = 0.0f;
	int minMsec;

	if( dedicated->integer )
		return;

	cls.realtime += realmsec;

	if( cls.demo.playing && cls.demo.play_ignore_next_frametime )
	{
		gamemsec = 0;
		cls.demo.play_ignore_next_frametime = qfalse;
	}

	// demoavi
	if( cl_demoavi_fps->modified )
	{
		float newvalue = 1000.0f / (int)( 1000.0f/cl_demoavi_fps->value );
		if( fabs( newvalue - cl_demoavi_fps->value ) > 0.001 )
			Com_Printf( "cl_demoavi_fps value has been adjusted to %.4f\n", newvalue );

		Cvar_SetValue( "cl_demoavi_fps", newvalue );
		cl_demoavi_fps->modified = qfalse;
	}

	if( ( cls.demo.avi || cls.demo.pending_avi ) && cls.state == CA_ACTIVE )
	{
		if( cls.demo.pending_avi && !cls.demo.avi )
		{
			cls.demo.pending_avi = qfalse;
			CL_BeginDemoAviDump();
		}

		// fixed time for next frame
		if( cls.demo.avi_video )
		{
			gamemsec = ( 1000.0 / (double)cl_demoavi_fps->integer ) * Cvar_Value( "timescale" );
			if( gamemsec < 1 )
				gamemsec = 1;
		}
	}

	cls.gametime += gamemsec;

	allRealMsec += realmsec;
	allGameMsec += gamemsec;

	if( cl_extrapolate->integer && !cls.demo.playing )
		cl.extrapolationTime = cl.snapFrameTime * 0.5;
	else
		cl.extrapolationTime = 0;

	CL_UpdateSnapshot();
	CL_AdjustServerTime( gamemsec );
	CL_UserInputFrame();
	CL_NetFrame( realmsec );

	if( cl_maxfps->integer > 0 && !cl_timedemo->integer && !( cls.demo.avi_video && cls.state == CA_ACTIVE ) )
	{
		// Vic: do not allow setting cl_maxfps to very low values to prevent cheating
		if( cl_maxfps->integer < 24 )
			Cvar_ForceSet( "cl_maxfps", "24" );

		minMsec = max( 1000.0f / cl_maxfps->value, 1 );
		roundingMsec += max( ( 1000.0f / cl_maxfps->value ), 1.0f ) - minMsec;
	}
	else
	{
		minMsec = 1;
		roundingMsec = 0;
	}

	if( roundingMsec >= 1.0f )
	{
		minMsec += (int)roundingMsec;
		roundingMsec -= (int)roundingMsec;
	}

	if( allRealMsec + extraMsec < minMsec )
	{
#ifdef PUTCPU2SLEEP
		if( cl_sleep->integer && minMsec - extraMsec > 1 )
			Sys_Sleep( 1 );
#endif
		return;
	}

	cls.frametime = (float)allGameMsec * 0.001f;
	cls.trueframetime = cls.frametime;
#if 1
	if( allRealMsec < minMsec ) // is compensating for a too slow frame
	{
		extraMsec -= ( minMsec - allRealMsec );
		clamp( extraMsec, 0, 100 );
	}
	else // too slow, or exact frame
	{
		extraMsec = allRealMsec - minMsec;
		clamp( extraMsec, 0, 100 );
	}
#else
	extraMsec = allRealMsec - minMsec;
	clamp( extraMsec, 0, minMsec );
#endif

	CL_TimedemoStats();

	// allow rendering DLL change
	VID_CheckChanges();

	cl.inputRefreshed = qfalse;
	if( cls.state != CA_ACTIVE )
		CL_UpdateCommandInput();

	CL_NewUserCommand( allRealMsec );

	// update the screen
	if( host_speeds->integer )
		time_before_ref = Sys_Milliseconds();
	SCR_UpdateScreen();
	if( host_speeds->integer )
		time_after_ref = Sys_Milliseconds();

	// this refresh will rarely do anything, it's just a call in case it wasn't called
	CL_UpdateCommandInput();

	if( CL_WriteAvi() )
	{
		int frame = ++cls.demo.avi_frame;
		if( cls.demo.avi_video )
			R_WriteAviFrame( frame, cl_demoavi_scissor->integer );
	}

	// update audio
	if( cls.state != CA_ACTIVE || SCR_GetCinematicTime() )
	{
		// if the loading plaque is up, clear everything out to make sure we aren't looping a dirty
		// dma buffer while loading
		if( cls.disable_screen )
			CL_SoundModule_Clear();
		else
			CL_SoundModule_Update( vec3_origin, vec3_origin, vec3_origin, vec3_origin, vec3_origin, qfalse );
	}

	// advance local effects for next frame
	SCR_RunCinematic();
	SCR_RunConsole( allRealMsec );

	allRealMsec = 0;
	allGameMsec = 0;

	cls.framecount++;
#ifdef _DEBUG
	CL_LogStats();
#endif
}


//============================================================================

/*
* CL_CheckForUpdate
*
* retrieve a file with the last version umber on a web server, compare with current version
* display a message box in case the user need to update
*/
static void CL_CheckForUpdate( void )
{
#ifdef PUBLIC_BUILD
	char url[MAX_STRING_CHARS];
	qboolean success;
	float local_version, net_version;
	FILE *f;

	// step one get the last version file
	Com_Printf( "Checking for " APPLICATION " update.\n" );

	Q_snprintfz( url, sizeof( url ), "%s%s", APP_UPDATE_URL, APP_UPDATE_FILE );
	success = Web_Get( url, NULL, APP_UPDATE_FILE, qfalse, 2, 2, NULL, qtrue );

	if( !success )
		return;

	// got the file
	// this look stupid but is the safe way to do it
	local_version = atof( va( "%4.3f", APP_VERSION ) );

	f = fopen( APP_UPDATE_FILE, "r" );

	if( f == NULL )
	{
		Com_Printf( "Fail to open last version file.\n" );
		return;
	}

	if( fscanf( f, "%f", &net_version ) != 1 )
	{
		// error
		fclose( f );
		remove( APP_UPDATE_FILE );
		Com_Printf( "Fail to parse last version file.\n" );
		return;
	}

	// we have the version
	//Com_Printf("CheckForUpdate: local: %f net: %f\n", local_version, net_version);
	if( net_version > local_version )
	{
		char cmd[1024];
		char net_version_str[16], *s;

		Q_snprintfz( net_version_str, sizeof( net_version_str ), "%4.3f", net_version );
		s = net_version_str + strlen( net_version_str ) - 1;
		while( *s == '0' ) s--;
		net_version_str[s-net_version_str+1] = '\0';

		// you should update
		Com_Printf( APPLICATION " version %s is available.\nVisit " APP_URL " for more information\n", net_version_str );
		Q_snprintfz( cmd, 1024, "menu_msgbox \"Version %s of " APPLICATION " is available\" \"Visit " APP_URL " for more information\"", net_version_str );
		Cbuf_ExecuteText( EXEC_APPEND, cmd );
	}
	else if( net_version == local_version )
	{
		Com_Printf( "Your %s version is up-to-date.\n", APPLICATION );
	}

	fclose( f );

	// cleanup
	remove( APP_UPDATE_FILE );
#endif // PUBLIC_BUILD
}

/*
* CL_Init
*/
void CL_Init( void )
{
	if( dedicated->integer )
		return; // nothing running on the client

	// all archived variables will now be loaded

	Con_Init();

#ifndef VID_INITFIRST
	CL_SoundModule_Init( qtrue );
	VID_Init();
#else
	VID_Init();
	CL_SoundModule_Init( qtrue ); // sound must be initialized after window is created
#endif

	CL_ClearState();

	SCR_InitScreen();
	cls.disable_screen = qtrue; // don't draw yet

	CL_InitLocal();
	CL_InitInput();

	CL_InitMedia();
	Cbuf_ExecuteText( EXEC_NOW, "menu_main" );

	// check for update
	CL_CheckForUpdate();
}

/*
* CL_Shutdown
* 
* FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
* to run quit through here before the final handoff to the sys code.
*/
void CL_Shutdown( void )
{
	static qboolean isdown = qfalse;

	if( cls.state == CA_UNINITIALIZED )
		return;
	if( isdown )
		return;

	isdown = qtrue;

	CL_WriteConfiguration( "config.cfg", qtrue );

	CL_UIModule_Shutdown();
	CL_GameModule_Shutdown();
	CL_SoundModule_Shutdown( qtrue );
	CL_ShutdownInput();
	VID_Shutdown();
}
