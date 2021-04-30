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
// cl_demo.c  -- demo recording

#include "client.h"

#ifdef WSWCURL
#include "../qcommon/wswcurl.h"
#endif
/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage( msg_t *msg )
{
	int len, swlen;

	if( cls.demo.file <= 0 )
	{
		cls.demo.recording = qfalse;
		return;
	}

	// the first eight bytes are just packet sequencing stuff
	len = msg->cursize - 8;
	swlen = LittleLong( len );

	// skip bad packets
	if( swlen )
	{
		FS_Write( &swlen, 4, cls.demo.file );
		FS_Write( msg->data + 8, len, cls.demo.file );
	}
}

/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f( void )
{
	int len, arg;
	qboolean silent, cancel;

	// look through all the args
	silent = qfalse;
	cancel = qfalse;
	for( arg = 1; arg < Cmd_Argc(); arg++ )
	{
		if( !Q_stricmp( Cmd_Argv( arg ), "silent" ) )
			silent = qtrue;
		else if( !Q_stricmp( Cmd_Argv( arg ), "cancel" ) )
			cancel = qtrue;
	}

	if( !cls.demo.recording )
	{
		if( !silent )
			Com_Printf( "Not recording a demo.\n" );
		return;
	}

	// finish up
	len = -1;
	FS_Write( &len, 4, cls.demo.file );

	// write the svc_demoinfo stuff
	len = FS_Tell( cls.demo.file );
	FS_Seek( cls.demo.file, 0 + sizeof(int) + 1, FS_SEEK_SET );

	FS_Write( &cls.demo.basetime, 4, cls.demo.file );
	FS_Write( &cls.demo.duration, 4, cls.demo.file );
	FS_Write( &len, 4, cls.demo.file );



	// cancel the demos
	if( cancel )
	{
		// remove the file that correspond to cls.demofile
		if( !silent )
			Com_Printf( "Canceling demo: %s\n", cls.demo.filename );
		if( !FS_RemoveFile( cls.demo.filename ) && !silent )
			Com_Printf( "Error canceling demo." );
	}

	if( !silent )
		Com_Printf( "Stopped demo: %s\n", cls.demo.filename );

	cls.demo.file = 0; // file id
	Mem_ZoneFree( cls.demo.filename );
	cls.demo.filename = NULL;
	cls.demo.recording = qfalse;

}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
#define DEMO_SAFEWRITE(buf,force) \
	if( force || buf.cursize > buf.maxsize / 2 ) \
{ \
	CL_WriteDemoMessage( &buf ); \
	buf.cursize = 8; \
}

void CL_Record_f( void )
{
	char *name;
	int i, name_size;
	char buf_data[MAX_MSGLEN];
	msg_t buf;
	int length;
	qboolean silent;
	purelist_t *purefile;

	if( cls.state != CA_ACTIVE )
	{
		Com_Printf( "You must be in a level to record.\n" );
		return;
	}

	if( Cmd_Argc() < 2 )
	{
		Com_Printf( "record <demoname>\n" );
		return;
	}

	if( Cmd_Argc() > 2 && !Q_stricmp( Cmd_Argv( 2 ), "silent" ) )
		silent = qtrue;
	else
		silent = qfalse;

	if( cls.demo.playing )
	{
		if( !silent )
			Com_Printf( "You can't record from another demo.\n" );
		return;
	}

	if( cls.demo.recording )
	{
		if( !silent )
			Com_Printf( "Already recording.\n" );
		return;
	}

	// open the demo file

	name_size = sizeof( char ) * ( strlen( "demos/" ) + strlen( Cmd_Argv( 1 ) ) + strlen( APP_DEMO_EXTENSION_STR ) + 1 );
	name = Mem_ZoneMalloc( name_size );

	Q_snprintfz( name, name_size, "demos/%s", Cmd_Argv( 1 ) );
	COM_SanitizeFilePath( name );
	COM_DefaultExtension( name, APP_DEMO_EXTENSION_STR, name_size );

	if( !COM_ValidateRelativeFilename( name ) )
	{
		if( !silent )
			Com_Printf( "Invalid filename.\n" );
		Mem_ZoneFree( name );
		return;
	}

	length = FS_FOpenFile( name, &cls.demo.file, FS_WRITE );
	if( length == -1 )
	{
		Com_Printf( "Error: Couldn't create the demo file.\n" );
		Mem_ZoneFree( name );
		return;
	}

	if( !silent )
		Com_Printf( "Recording demo: %s\n", name );

	// store the name in case we need it later
	cls.demo.filename = name;
	cls.demo.recording = qtrue;
	cls.demo.basetime = cls.demo.duration = 0;

	// don't start saving messages until a non-delta compressed message is received
	cls.demo.waiting = qtrue;

	//
	// write out messages to hold the startup information
	//
	MSG_Init( &buf, (qbyte *)buf_data, sizeof( buf_data ) );

	// dummy packet sequencing stuff
	MSG_WriteLong( &buf, 0 );
	MSG_WriteLong( &buf, 0 );

	// write empty demoinfo, we'll override the values later
	MSG_WriteByte( &buf, svc_demoinfo );
	MSG_WriteLong( &buf, 0 );	// initial server time
	MSG_WriteLong( &buf, 0 );	// demo duration in milliseconds
	MSG_WriteLong( &buf, 0 );	// file offset at which the final is written

	// send the serverdata
	MSG_WriteByte( &buf, svc_serverdata );
	MSG_WriteLong( &buf, APP_PROTOCOL_VERSION );
	MSG_WriteLong( &buf, 0x10000 + cl.servercount );
	MSG_WriteShort( &buf, (unsigned short)cl.snapFrameTime );
	MSG_WriteString( &buf, FS_BaseGameDirectory() );
	MSG_WriteString( &buf, FS_GameDirectory() );
	MSG_WriteShort( &buf, cls.sv_maxclients );
	MSG_WriteShort( &buf, cl.playernum );
	MSG_WriteString( &buf, cl.servermessage );
	MSG_WriteByte( &buf, 0 ); // no sv_bitflags (no pure)

	// always write purelist
	i = Com_CountPureListFiles( cls.purelist );
	if( i > (short)0x7fff )
		Com_Error( ERR_DROP, "CL_Record_f: Too many pure files." );

	MSG_WriteShort( &buf, i );

	purefile = cls.purelist;
	while( purefile )
	{
		MSG_WriteString( &buf, purefile->filename );
		MSG_WriteLong( &buf, purefile->checksum );
		purefile = purefile->next;
		DEMO_SAFEWRITE( buf, qfalse );
	}

	// configstrings
	length = 0;
	for( i = 0; i < MAX_CONFIGSTRINGS; i++ )
	{
		if( cl.configstrings[i][0] )
			length++;
	}

	if( length )
	{
		MSG_WriteByte( &buf, svc_servercmd );
		MSG_WriteLong( &buf, 0 );
		MSG_WriteShort( &buf, length );
		DEMO_SAFEWRITE( buf, qfalse );

		for( i = 0; i < MAX_CONFIGSTRINGS; i++ )
		{
			if( cl.configstrings[i][0] )
				MSG_WriteString( &buf, va( "cs %i \"%s\"", i, cl.configstrings[i] ) );

			DEMO_SAFEWRITE( buf, qfalse );
		}
	}

	// baselines
	{
		entity_state_t *ent;
		memset( &nullEntityState, 0, sizeof( nullEntityState ) );

		for( i = 0; i < MAX_EDICTS; i++ )
		{
			ent = &cl.snapsData.baselines[i];
			if( !Com_EntityIsBaseLined( ent ) )
				continue;

			MSG_WriteByte( &buf, svc_spawnbaseline );
			Com_WriteDeltaEntityState( &nullEntityState, &cl.snapsData.baselines[i], &buf, qtrue );
			
			DEMO_SAFEWRITE( buf, qfalse );
		}
	}

	// write a command to launch the precache process
	MSG_WriteByte( &buf, svc_servercmd );
	MSG_WriteLong( &buf, 0 );
	MSG_WriteShort( &buf, 1 );
	MSG_WriteString( &buf, "precache" );
	DEMO_SAFEWRITE( buf, qtrue );

	// the rest of the demo file will be individual frames
}


//================================================================
//
//	CLIENT SIDE DEMO PLAYBACK
//
//================================================================

#define DEMO_HTTP_OK			200
#define DEMO_HTTP_READAHEAD		1024*50

// demo file
int demofilehandle;
static int demofilelen, demofilelentotal;

// demo message
qbyte msgbuf[MAX_MSGLEN];
msg_t demomsg;

#ifdef WSWCURL
static wswcurl_req *democurlrequest;

static void CL_CurlDemoDoneCallback( wswcurl_req *req, int status )
{
	if( req->respcode != DEMO_HTTP_OK )
		Com_Printf( "No valid demo file found\n" );
}

static void CL_CurlDemoProgressCallback( wswcurl_req *req, double percentage )
{
	int offset = 0;

	// flush data to disk
	FS_Flush( req->filenum );

	if( demofilehandle )
	{
		offset = FS_Tell( demofilehandle );
		FS_FCloseFile( demofilehandle );
	}

	demofilelentotal = FS_FOpenFile( req->filename, &demofilehandle, FS_READ );
	demofilelen = demofilelentotal;
	FS_Seek( demofilehandle, offset, FS_SEEK_SET );
}
#endif

/*
=================
CL_BeginDemoAviDump
=================
*/
void CL_BeginDemoAviDump( void )
{
	if( cls.demo.avi )
		return;

	cls.demo.avi_video = (cl_demoavi_video->integer ? qtrue : qfalse);
	cls.demo.avi_audio = (cl_demoavi_audio->integer ? qtrue : qfalse);
	cls.demo.avi = (cls.demo.avi_video || cls.demo.avi_audio);
	cls.demo.avi_frame = 0;

	if( cls.demo.avi_video )
		R_BeginAviDemo();

	if( cls.demo.avi_audio )
		CL_SoundModule_BeginAviDemo();
}

/*
=================
CL_StopDemoAviDump
=================
*/
static void CL_StopDemoAviDump( void )
{
	if( !cls.demo.avi )
		return;

	if( cls.demo.avi_video )
	{
		R_StopAviDemo();
		cls.demo.avi_video = qfalse;
	}

	if( cls.demo.avi_audio )
	{
		CL_SoundModule_StopAviDemo();
		cls.demo.avi_audio = qfalse;
	}

	cls.demo.avi = qfalse;
	cls.demo.avi_frame = 0;
}

/*
=================
CL_DemoCompleted

Close the demo file and disable demo state. Called from disconnection proccess
=================
*/
void CL_DemoCompleted( void )
{

	if( cls.demo.avi )
		CL_StopDemoAviDump();

	if( demofilehandle )
	{
		FS_FCloseFile( demofilehandle );
		demofilehandle = 0;
	}

	demofilelen = demofilelentotal = 0;
#ifdef WSWCURL
	if( democurlrequest )
	{
		char *tmpname = NULL;

		if( democurlrequest->filename )
			tmpname = TempCopyString( democurlrequest->filename );

		wswcurl_delete( democurlrequest );
		democurlrequest = NULL;

		if( tmpname )
		{
			FS_RemoveFile( tmpname );
			Mem_TempFree( tmpname );
		}
	}
#endif
	cls.demo.playing = qfalse;
	cls.demo.basetime = cls.demo.duration = 0;

	Com_SetDemoPlaying( qfalse );

	Cvar_ForceSet( "demofile", "" );
	Cvar_ForceSet( "demotime", "0" );
	Cvar_ForceSet( "demoduration", "0" );

	Com_Printf( "Demo completed\n" );

	memset( &cls.demo, 0, sizeof( cls.demo ) );
}

/*
=================
CL_ReadDemoMessage

Read a packet from the demo file and send it to the messages parser
=================
*/
static void CL_ReadDemoMessage( void )
{
	int msglen, read;
#ifdef WSWCURL
	if( democurlrequest && democurlrequest->respcode && democurlrequest->respcode != DEMO_HTTP_OK )
	{
		CL_Disconnect( NULL );
		return;
	}
#endif
	if( !demofilehandle )
	{
		CL_Disconnect( NULL );
		return;
	}

	if( demofilelen <= 0 )
	{
		CL_Disconnect( NULL );
		return;
	}

	msglen = 0;

	// get the next message
	FS_Read( &msglen, 4, demofilehandle );
	demofilelen -= 4;

	msglen = LittleLong( msglen );
	if( msglen == -1 )
	{
		CL_Disconnect( NULL );
		return;
	}

	if( msglen > MAX_MSGLEN )
		Com_Error( ERR_DROP, "SV_SendClientMessages: msglen > MAX_MSGLEN" );

	if( demofilelen <= 0 )
	{
		CL_Disconnect( NULL );
		return;
	}

	read = FS_Read( msgbuf, msglen, demofilehandle );
	if( read != msglen )
		Com_Error( ERR_DROP, "Error reading demo file: End of file" );

	demomsg.maxsize = sizeof( msgbuf );
	demomsg.data = msgbuf;
	demomsg.cursize = msglen;
	demomsg.readcount = 0;

	CL_ParseServerMessage( &demomsg );
}

/*
=================
CL_ReadDemoPackets

See if it's time to read a new demo packet
=================
*/
void CL_ReadDemoPackets( void )
{
	while( cls.demo.playing && ( cl.receivedSnapNum <= 0 || ( cl.snapShots[cl.receivedSnapNum & SNAPS_BACKUP_MASK].timeStamp < cl.serverTime ) ) )
	{
#ifdef WSWCURL
		// if downloading via HTTP, wait until enough data arrives
		if( democurlrequest && !democurlrequest->respcode 
			&& (!demofilehandle || demofilelen < FS_Tell( demofilehandle ) + DEMO_HTTP_READAHEAD ) )
			return;
#endif
		CL_ReadDemoMessage();
	}

	Cvar_ForceSet( "demoduration", va( "%i", (int)ceil( cls.demo.duration/1000.0f ) ) );
	Cvar_ForceSet( "demotime", va( "%i", (int)floor( max( cl.snapShots[cl.receivedSnapNum & SNAPS_BACKUP_MASK].timeStamp - cls.demo.basetime,0 ) /1000.0f ) ) );

	if( cls.demo.play_jump )
		cls.demo.play_jump = qfalse;
}

/*
====================
CL_StartDemo
====================
*/
static void CL_StartDemo( const char *demoname )
{
	size_t name_size;
	char *name, *servername;
	int tempdemofilehandle = 0, tempdemofilelen = -1;
#ifdef WSWCURL
	wswcurl_req *tempdemocurlrequest = NULL;
#endif

	// have to copy the argument now, since next actions will lose it
	servername = TempCopyString( demoname );
	COM_SanitizeFilePath( servername );

	if( !strncmp( demoname, "http://", 7 ) )
	{
		int i;

		// keep filenames random
		name_size = sizeof( char ) * ( strlen( "demos/" ) + strlen( COM_FileBase( servername ) ) + 5 + strlen( ".tmp" ) + 1 );
		name = Mem_TempMalloc( name_size );

		for( i = 3; i > 0; i-- )
		{
			int randomizer;

			randomizer = brandom( 0, 9999 );

			Q_snprintfz( name, name_size, "demos/%s", COM_FileBase( servername ) );
			COM_StripExtension( name );
			Q_strncatz( name, va( "_%i.tmp", randomizer ), name_size );
			if( FS_FOpenFile( name, NULL, FS_READ ) == -1 )
				break;
		}

		if( !i )
		{
			Com_Printf( "Could not create temp file for demo\n" );
			Mem_TempFree( name );
			return;
		}
	}
	else
	{
		name_size = sizeof( char ) * ( strlen( "demos/" ) + strlen( servername ) + strlen( APP_DEMO_EXTENSION_STR ) + 1 );
		name = Mem_TempMalloc( name_size );

		Q_snprintfz( name, name_size, "demos/%s", servername );
		COM_DefaultExtension( name, APP_DEMO_EXTENSION_STR, name_size );
		if( COM_ValidateRelativeFilename( name ) )
			tempdemofilelen = FS_FOpenFile( name, &tempdemofilehandle, FS_READ );	// open the demo file

		if( !tempdemofilehandle || tempdemofilelen < 1 )
		{
			// relative filename didn't work, try launching a demo from absolute path
			Q_snprintfz( name, name_size, "%s", servername );
			COM_DefaultExtension( name, APP_DEMO_EXTENSION_STR, name_size );
			tempdemofilelen = FS_FOpenAbsoluteFile( name, &tempdemofilehandle, FS_READ );
		}

		if( !tempdemofilehandle || tempdemofilelen < 1 )
		{
			Com_Printf( "No valid demo file found\n" );
			FS_FCloseFile( tempdemofilehandle );
			Mem_TempFree( name );
			Mem_TempFree( servername );
			return;
		}
	}

	// make sure a local server is killed
	Cbuf_ExecuteText( EXEC_NOW, "killserver\n" );
	CL_Disconnect( NULL );

	memset( &cls.demo, 0, sizeof( cls.demo ) );

	demofilehandle = tempdemofilehandle;
	demofilelentotal = tempdemofilelen;
	demofilelen = demofilelentotal;
#ifdef WSWCURL
	democurlrequest = tempdemocurlrequest;
#endif
	cls.servername = ZoneCopyString( COM_FileBase( servername ) );
	COM_StripExtension( cls.servername );

	CL_SetClientState( CA_HANDSHAKE );
	Com_SetDemoPlaying( qtrue );
	cls.demo.playing = qtrue;
	cls.demo.basetime = cls.demo.duration = 0;

	cls.demo.play_ignore_next_frametime = qfalse;
	cls.demo.play_jump = qfalse;

	// demo info cvars
	Cvar_Get( "demotime", "0", CVAR_READONLY );
	Cvar_Get( "demoduration", "0", CVAR_READONLY );
	Cvar_ForceSet( "demofile", name );

	// set up for timedemo settings
	memset( &cl.timedemo, 0, sizeof( cl.timedemo ) );

	Mem_TempFree( name );
	Mem_TempFree( servername );
}

/*
====================
CL_PlayDemo_f

demo <demoname>
====================
*/
void CL_PlayDemo_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "demo <demoname>\n" );
		return;
	}
	CL_StartDemo( Cmd_Argv( 1 ) );
}

/*
====================
CL_DemoJump_f
====================
*/
void CL_DemoJump_f( void )
{
	qboolean relative;
	int time;
	char *p;

	if( !cls.demo.playing )
	{
		Com_Printf( "Can only demojump when playing a demo\n" );
		return;
	}

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "Usage: demojump <time>\n" );
		Com_Printf( "Time format is [minutes:]seconds\n" );
		Com_Printf( "Use '+' or '-' in front of the time to specify it in relation to current position\n" );
		return;
	}

	p = Cmd_Argv( 1 );

	if( Cmd_Argv( 1 )[0] == '+' || Cmd_Argv( 1 )[0] == '-' )
	{
		relative = qtrue;
		p++;
	}
	else
	{
		relative = qfalse;
	}

	if( strchr( p, ':' ) )
		time = ( atoi( p ) * 60 + atoi( strchr( p, ':' ) + 1 ) ) * 1000;
	else
		time = atoi( p ) * 1000;

	if( Cmd_Argv( 1 )[0] == '-' )
		time = -time;
#ifdef WSWCURL
	if( democurlrequest && !democurlrequest->respcode && (relative ? time > 0 : (unsigned)time >= cls.gametime) )
	{
		Com_Printf( "Can not demojump forward while remote demo download is in progress\n" );
		return;
	}
#endif
	CL_SoundModule_StopAllSounds();

	if( relative )
		cls.gametime += time;
	else
		cls.gametime = time; // gametime always starts from 0
	
	if( cl.serverTime < cl.snapShots[cl.receivedSnapNum&SNAPS_BACKUP_MASK].timeStamp )
		cl.pendingSnapNum = 0;

	CL_AdjustServerTime( 1 );

	if( cl.serverTime < cl.snapShots[cl.receivedSnapNum&SNAPS_BACKUP_MASK].timeStamp )
	{
		demofilelen = demofilelentotal;
		FS_Seek( demofilehandle, 0, FS_SEEK_SET );
		cl.currentSnapNum = cl.receivedSnapNum = 0;
	}

	cls.demo.play_jump = qtrue;
}

/*
====================
CL_PlayDemoToAvi_f

demoavi <demoname> (if no name suplied, toogles demoavi status)
====================
*/
void CL_PlayDemoToAvi_f( void )
{
	if( Cmd_Argc() == 1 && cls.demo.playing ) // toggle demoavi mode
	{
		if( !cls.demo.avi )
			CL_BeginDemoAviDump();
		else
			CL_StopDemoAviDump();
	}
	else if( Cmd_Argc() == 2 )
	{
		char *tempname = TempCopyString( Cmd_Argv( 1 ) );

		CL_StartDemo( tempname );
		if( cls.demo.playing )
			cls.demo.pending_avi = qtrue;

		Mem_TempFree( tempname );
	}
	else
	{
		Com_Printf( "Usage: %sdemoavi <demoname>%s or %sdemoavi%s while playing a demo\n",
			S_COLOR_YELLOW, S_COLOR_WHITE, S_COLOR_YELLOW, S_COLOR_WHITE );
	}
}

