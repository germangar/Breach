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
// sv_main.c -- server main program

#include "server.h"

// shared message buffer to be used for occasional messages
msg_t tmpMessage;
qbyte tmpMessageData[MAX_MSGLEN];



//=============================================================================
//
//Com_Printf redirection
//
//=============================================================================

char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
void SV_FlushRedirect( int sv_redirected, char *outputbuf )
{
	if( sv_redirected == RD_PACKET )
	{
		Netchan_OutOfBandPrint( NS_SERVER, net_from, "print\n%s", outputbuf );
	}
}


//=============================================================================
//
//EVENT MESSAGES
//
//=============================================================================

//======================
//SV_AddServerCommand
//
//The given command will be transmitted to the client, and is guaranteed to
//not have future snapshot_t executed before it is executed
//======================
void SV_AddServerCommand( client_t *client, const char *cmd )
{
	int index;
	unsigned int i;

	if( !client )
		return;

	if( client->fakeClient )
		return;

	if( !cmd || !cmd[0] || !strlen( cmd ) )
		return;

	client->reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged, we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient() doesn't cause a recursive drop client
	if( client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1 )
	{
		//Com_Printf( "===== pending server commands =====\n" );
		for( i = client->reliableAcknowledge + 1; i <= client->reliableSequence; i++ )
		{
			Com_DPrintf( "cmd %5d: %s\n", i, client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] );
		}
		Com_DPrintf( "cmd %5d: %s\n", i, cmd );
		SV_DropClient( client, DROP_TYPE_GENERAL, "Server command overflow" );
		return;
	}
	index = client->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( client->reliableCommands[index], cmd, sizeof( client->reliableCommands[index] ) );
}

//=================
//SV_SendServerCommand
//
//Sends a reliable command string to be interpreted by
//the client: "cs", "changing", "disconnect", etc
//A NULL client will broadcast to all clients
//=================
void SV_SendServerCommand( client_t *cl, const char *format, ... )
{
	va_list	argptr;
	char message[MAX_MSGLEN];
	client_t *client;
	int j;

	va_start( argptr, format );
	Q_vsnprintfz( message, sizeof( message ), format, argptr );
	va_end( argptr );

	if( cl != NULL )
	{
		if( cl->state < CS_CONNECTING )
			return;
		SV_AddServerCommand( cl, message );
		return;
	}

	// send the data to all relevant clients
	for( j = 0, client = svs.clients; j < sv_maxclients->integer; j++, client++ )
	{
		if( client->state < CS_CONNECTING )
			continue;
		SV_AddServerCommand( client, message );
	}
}

//==================
//SV_AddReliableCommandsToMessage
//
//(re)send all server commands the client hasn't acknowledged yet
//==================
void SV_AddReliableCommandsToMessage( client_t *client, msg_t *msg )
{
	unsigned int i;

	if( client->fakeClient )
		return;

	if( sv_debug_serverCmd->integer > 1 )
		Com_Printf( "sv_cl->reliableAcknowledge: %i sv_cl->reliableSequence:%i\n", client->reliableAcknowledge, client->reliableSequence );

	if( !client->reliableSequence )
		return;

	if( (client->reliableSequence - client->reliableAcknowledge) == 0 )
		return;

	MSG_WriteByte( msg, svc_servercmd );
	MSG_WriteLong( msg, client->reliableAcknowledge + 1 );
	MSG_WriteShort( msg, client->reliableSequence - client->reliableAcknowledge );
	if( sv_debug_serverCmd->integer )
		Com_Printf( "SV_AddServerCommandsToMessage: Writing %i reliable cmds\n", (int)( client->reliableSequence - client->reliableAcknowledge ) );

	for( i = client->reliableAcknowledge + 1; i <= client->reliableSequence; i++ )
	{
		if( !strlen( client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] ) )
		{
			SV_DropClient( client, DROP_TYPE_GENERAL, "SV_AddReliableCommandsToMessage: Tried to write empty reliable cmd\n" );
			return;
		}

		MSG_WriteString( msg, client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] );
	}

	client->reliableSent = client->reliableSequence;
}

//=================
//SV_BroadcastCommand
//
//Sends a command to all connected clients.
//=================
void SV_BroadcastCommand( const char *format, ... )
{
	client_t *client;
	int i;
	va_list	argptr;
	char string[1024];

	if( !sv.state )
		return;

	va_start( argptr, format );
	Q_vsnprintfz( string, sizeof( string ), format, argptr );
	va_end( argptr );

	for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
	{
		if( client->state < CS_CONNECTING )
			continue;
		SV_SendServerCommand( client, string );
	}
}

//=============================================================================
//
//EVENT MESSAGES
//
//=============================================================================

//=================
//SV_ClientChatf
//
//Sends text across to be displayed in chat window
//=================
void SV_ClientChatf( client_t *cl, const char *format, ... )
{
	va_list	argptr;
	char string[1024], *p;
	client_t *client;
	int i;

	va_start( argptr, format );
	Q_vsnprintfz( string, sizeof( string ), format, argptr );
	va_end( argptr );

	// double quotes are bad
	while( ( p = strchr( string, '\"' ) ) != NULL )
		*p = '\'';

	if( cl != NULL )
	{
		if( cl->state == CS_SPAWNED )
			SV_SendServerCommand( cl, "ch \"%s\"", string );
		return;
	}

	for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
	{
		if( client->state == CS_SPAWNED )
			SV_SendServerCommand( client, "ch \"%s\"", string );
	}

	// echo to console
	if( dedicated->integer )
	{
		char copy[MAX_PRINTMSG];
		int i;

		// mask off high bits and colored strings
		for( i = 0; i < sizeof( copy )-1 && string[i]; i++ )
			copy[i] = string[i]&127;
		copy[i] = 0;
		Com_Printf( "%s", copy );
	}
}

//=================
//SV_ClientPrintf
//
//Sends text across to be displayed as print
//=================
void SV_ClientPrintf( client_t *cl, const char *format, ... )
{
	va_list	argptr;
	char string[1024], *p;
	client_t *client;
	int i;

	va_start( argptr, format );
	Q_vsnprintfz( string, sizeof( string ), format, argptr );
	va_end( argptr );

	// double quotes are bad
	while( ( p = strchr( string, '\"' ) ) != NULL )
		*p = '\'';

	if( cl != NULL )
	{
		SV_SendServerCommand( cl, "pr \"%s\"", string );
		return;
	}

	for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
	{
		if( client->state < CS_SPAWNED )
			continue;
		SV_SendServerCommand( client, "pr \"%s\"", string );
	}

	// echo to console
	if( dedicated->integer )
	{
		char copy[MAX_PRINTMSG];
		int i;

		// mask off high bits and colored strings
		for( i = 0; i < sizeof( copy )-1 && string[i]; i++ )
			copy[i] = string[i]&127;
		copy[i] = 0;
		Com_Printf( "%s", copy );
	}
}

//===============================================================================
//
//FRAME UPDATES
//
//===============================================================================


//=======================
//SV_SendClientsFragments
//=======================
qboolean SV_SendClientsFragments( void )
{
	int i;
	client_t *client;
	qboolean sent = qfalse;
	qboolean remaining = qfalse;

	// send a message to each connected client
	for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
	{
		if( !client->state )
			continue;

		if( client->fakeClient )
			continue;

		if( !client->netchan.unsentFragments )
			continue;

		Netchan_TransmitNextFragment( &client->netchan );
		sent = qtrue;

		if( client->netchan.unsentFragments )
			remaining = qtrue;
	}

	return sent;
}

//==================
//SV_Netchan_Transmit
//==================
static void SV_Netchan_Transmit( client_t *client, msg_t *msg )
{
	int zerror;

	// fire all unsent fragments now
	Netchan_PushAllFragments( &client->netchan );

	if( sv_compresspackets->integer )
	{
		zerror = Netchan_CompressMessage( msg );
		if( zerror < 0 )
		{          // it's compression error, just send uncompressed
			Com_DPrintf( "SV_Netchan_Transmit (ignoring compression): Compression error %i\n", zerror );
		}
	}

	Netchan_Transmit( &client->netchan, msg );
	client->lastPacketSentTime = svs.realtime;
}

//=======================
//SV_InitClientMessage
//=======================
void SV_InitClientMessage( client_t *client, msg_t *msg, qbyte *data, size_t size )
{
	assert( client );
	if( client->fakeClient )
		return;

	if( data && size )  // otherwise we are reusing an already initialized msg
		MSG_Init( msg, data, size );
	MSG_Clear( msg );

	// write the last client-command we received so it's acknowledged
	MSG_WriteByte( msg, svc_clcack );
	MSG_WriteLong( msg, client->clientCommandExecuted ); // acknowledge reliable command
	MSG_WriteLong( msg, client->UcmdReceived ); // acknowledge  usercmd
}

//=======================
//SV_SendMessageToClient
//=======================
void SV_SendMessageToClient( client_t *client, msg_t *msg )
{
	assert( client && msg );
	if( client->fakeClient )
		return;

	SV_Netchan_Transmit( client, msg );
}

//=======================
//SV_SendClientMessages
//=======================
void SV_SendClientMessages( void )
{
	int i;
	client_t *client;

	// send a message to each connected client
	for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
	{
		if( !client->state )
			continue;

		if( client->fakeClient )
			continue;

		SV_UpdateActivity();

		if( client->state == CS_SPAWNED )
		{
			SV_InitClientMessage( client, &tmpMessage, NULL, 0 );

			SV_AddReliableCommandsToMessage( client, &tmpMessage );

			// send a snapshot
			SV_BuildClientFrameSnap( client );
			SV_WriteFrameSnapToClient( client, &tmpMessage );
			SV_SendMessageToClient( client, &tmpMessage );
		}
		else
		{
			// send pending reliable commands, or send heartbeats for not timing out
			if( client->reliableSequence > client->reliableAcknowledge ||
			    svs.realtime - client->lastPacketSentTime > 1000 )
			{
				SV_InitClientMessage( client, &tmpMessage, NULL, 0 );
				SV_AddReliableCommandsToMessage( client, &tmpMessage );
				SV_SendMessageToClient( client, &tmpMessage );
			}
		}
	}
}
