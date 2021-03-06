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
// cl_parse.c  -- parse a message received from the server

#include "client.h"
#include "../qcommon/webdownload.h"

char *svc_strings[256] =
{
	"svc_bad",
	"svc_nop",
	"svc_servercmd",
	"svc_serverdata",
	"svc_spawnbaseline",
	"svc_download",
	"svc_playerinfo",
	"svc_packetentities",
	"svc_clcack",
	"svc_gameinfo",
	"svc_frame",
	"svc_demoinfo",
	"svc_extension"
};

//=============================================================================

/*
===============
CL_CanDownloadModules

The user has to give permission for modules to be downloaded
===============
*/
qboolean CL_CanDownloadModules( void )
{
#if 0
	if( !Q_stricmp( FS_GameDirectory(), FS_BaseGameDirectory() ) )
	{
		Com_Error( ERR_DROP, "Can not download modules to the base directory" );
		return qfalse;
	}
#endif
	if( !cl_download_allow_modules->integer )
	{
		Com_Error( ERR_DROP, "Downloading of modules disabled." );
		return qfalse;
	}

	return qtrue;
}


/*
===============
CL_DownloadRequest

Request file download
return qfalse if couldn't request it for some reason
Files with .pk3 extension have to have gamedir attached
Other files must not have gamedir
===============
*/
qboolean CL_DownloadRequest( const char *filename, qboolean requestpak )
{
	if( cls.download.requestname )
	{
		Com_Printf( "Can't download: %s. Download already in progress.\n", filename );
		return qfalse;
	}

	if( !COM_ValidateRelativeFilename( filename ) )
	{
		Com_Printf( "Can't download: %s. Invalid filename.\n", filename );
		return qfalse;
	}

	if( FS_CheckPakExtension( filename ) )
	{
		if( FS_FOpenBaseFile( filename, NULL, FS_READ ) != -1 )
		{
			Com_Printf( "Can't download: %s. File already exists.\n", filename );
			return qfalse;
		}

		if( !Q_strnicmp( COM_FileBase( filename ), "modules", strlen( "modules" ) ) )
		{
			if( !CL_CanDownloadModules() )
			{
				return qfalse;
			}
		}
	}
	else
	{
		if( FS_FOpenFile( filename, NULL, FS_READ ) != -1 )
		{
			Com_Printf( "Can't download: %s. File already exists.\n", filename );
			return qfalse;
		}
	}

	Com_Printf( "Asking to download: %s\n", filename );

	cls.download.requestpak = requestpak;
	cls.download.requestname = Mem_ZoneMalloc( sizeof( char ) * ( strlen( filename ) + 1 ) );
	cls.download.timeout = Sys_Milliseconds() + 5000;
	Q_strncpyz( cls.download.requestname, filename, sizeof( char ) * ( strlen( filename ) + 1 ) );
	CL_AddReliableCommand( va( "download %i \"%s\"", requestpak, filename ) );

	return qtrue;
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists or couldn't send download request
Files with .pk3 extension have to have gamedir attached
Other files must not have gamedir
===============
*/
qboolean CL_CheckOrDownloadFile( const char *filename )
{
	if( !cl_downloads->integer )
		return qtrue;

	if( !COM_ValidateRelativeFilename( filename ) )
		return qtrue;

	if( FS_CheckPakExtension( filename ) )
	{
		if( FS_FOpenBaseFile( filename, NULL, FS_READ ) != -1 )
			return qtrue;
	}
	else
	{
		if( FS_FOpenFile( filename, NULL, FS_READ ) != -1 )
			return qtrue;
	}

	if( !CL_DownloadRequest( filename, qtrue ) )
		return qtrue;

	cls.download.requestnext = qtrue; // call CL_RequestNextDownload when done

	return qfalse;
}

/*
=====================
CL_WebDownloadProgress
Callback function for webdownloads.
Since Web_Get only returns once it's done, we have to do various things here:
Update download percent, handle input, redraw UI and send net packets.
=====================
*/
static int CL_WebDownloadProgress( double percent )
{
	cls.download.percent = percent;

	Cvar_ForceSet( "cl_download_percent", va( "%.1f", cls.download.percent * 100 ) );

	IN_Frame();
	CL_SendMessagesToServer( qfalse );
	//	CL_UIModule_DrawConnectScreen();
	SCR_UpdateScreen();

	// get new key events
	Sys_SendKeyEvents();
	// allow mice or other external controllers to add commands
	IN_Commands();
	// process console commands
	Cbuf_Execute();

	return cls.download.disconnect; // abort if disconnect received
}

/*
=====================
CL_DownloadComplete

Checks downloaded file's checksum, renames it and adds to the filesystem.
=====================
*/
static void CL_DownloadComplete( void )
{
	unsigned checksum;
	qbyte *buffer;
	int length;
	qboolean updateMapList = qfalse;

	// verify checksum
	if( FS_CheckPakExtension( cls.download.name ) ) 
	{
		if( !FS_IsPakValid( cls.download.tempname, &checksum ) ) 
		{
			Com_Printf( "Downloaded file is not a valid pack file. Removing\n" );
			FS_RemoveBaseFile( cls.download.tempname );
			return;
		}
	} 
	else 
	{
		length = FS_LoadBaseFile( cls.download.tempname, (void **)&buffer, NULL, 0 );
		if( !buffer ) 
		{
			Com_Printf( "Error: Couldn't load downloaded file\n" );
			return;
		}
		checksum = Com_BlockChecksum( buffer, length );
		FS_FreeBaseFile( buffer );
	}

	if( cls.download.checksum != checksum )
	{
		Com_Printf( "Downloaded file has wrong checksum. Removing\n" );
		FS_RemoveBaseFile( cls.download.tempname );
		return;
	}

	if( !FS_MoveBaseFile( cls.download.tempname, cls.download.name ) )
	{
		Com_Printf( "Failed to rename the downloaded file\n" );
		return;
	}

	if( FS_CheckPakExtension( cls.download.name ) )
		updateMapList = qtrue;
	else if( !Q_stricmp( COM_FileExtension( cls.download.name ), ".bsp" ) )
		updateMapList = qtrue;

	// Maplist hook so we also know when a new map is added
	if( updateMapList )
		ML_Update();

	cls.download.successCount++;
}

/*
=====================
CL_FreeDownloadList
=====================
*/
void CL_FreeDownloadList( void )
{
	download_list_t	*next;

	while( cls.download.list )
	{
		next = cls.download.list->next;
		Mem_ZoneFree( cls.download.list->filename );
		Mem_ZoneFree( cls.download.list );
		cls.download.list = next;
	}
}

/*
=====================
CL_DownloadDone
=====================
*/
void CL_DownloadDone( void )
{
	qboolean requestnext;

	Mem_ZoneFree( cls.download.requestname );
	cls.download.requestname = NULL;

	requestnext = cls.download.requestnext;
	cls.download.requestnext = qfalse;
	cls.download.requestpak = qfalse;
	cls.download.timeout = 0;
	cls.download.timestart = 0;
	cls.download.offset = cls.download.baseoffset = 0;
	cls.download.web = qfalse;
	cls.download.filenum = 0;

	// the server has changed map during the download
	if( cls.download.pending_reconnect )
	{
		cls.download.pending_reconnect = qfalse;
		CL_FreeDownloadList();
		CL_ServerReconnect_f();
		return;
	}

	if( requestnext && cls.state > CA_DISCONNECTED )
		CL_RequestNextDownload();
}

/*
=====================
CL_InitDownload_f

Hanldles server's initdownload message, starts web or server download if possible
=====================
*/
static void CL_InitDownload_f( void )
{
	const char *filename;
	const char *url;
	int size, alloc_size;
	unsigned checksum;
	qboolean allow_serverdownload;
	download_list_t	*dl;

	if( cls.demo.playing )
	{
		// ignore download commands coming from demo files
		return;
	}

	// read the data
	filename = Cmd_Argv( 1 );
	size = atoi( Cmd_Argv( 2 ) );
	checksum = strtoul( Cmd_Argv( 3 ), NULL, 10 );
	allow_serverdownload = ( atoi( Cmd_Argv( 4 ) ) != 0 );
	url = Cmd_Argv( 5 );

	if( !cls.download.requestname )
	{
		Com_Printf( "Got init download message without request\n" );
		return;
	}

	if( cls.download.filenum || cls.download.web )
	{
		Com_Printf( "Got init download message while already downloading\n" );
		return;
	}

	if( size == -1 )
	{              // means that download was refused
		Com_Printf( "Server refused download request: %s\n", url ); // if it's refused, url field holds the reason
		CL_DownloadDone();
		return;
	}

	if( allow_serverdownload == qfalse && strlen( url ) == 0 )
	{
		Com_Printf( "Neither server or web download provided by the server\n" );
		CL_DownloadDone();
		return;
	}

	if( size <= 0 )
	{
		Com_Printf( "Server gave invalid size, not downloading\n" );
		CL_DownloadDone();
		return;
	}

	if( checksum == 0 )
	{
		Com_Printf( "Server didn't provide checksum, not downloading\n" );
		CL_DownloadDone();
		return;
	}

	if( !COM_ValidateRelativeFilename( filename ) )
	{
		Com_Printf( "Not downloading, invalid filename: %s\n", filename );
		CL_DownloadDone();
		return;
	}

	if( !cl_downloads_from_web->integer && !allow_serverdownload )
	{
		Com_Printf( "Not downloading, server only provided web download\n" );
		CL_DownloadDone();
		return;
	}

	if( FS_CheckPakExtension( filename ) && !cls.download.requestpak )
	{
		Com_Printf( "Got a pak file when requesting normal one, not downloading\n" );
		CL_DownloadDone();
		return;
	}

	if( !FS_CheckPakExtension( filename ) && cls.download.requestpak )
	{
		Com_Printf( "Got non pak file when requesting pak, not downloading\n" );
		CL_DownloadDone();
		return;
	}

	if( !strchr( filename, '/' ) )
	{
		Com_Printf( "Refusing to download file with no gamedir: %s\n", filename );
		CL_DownloadDone();
		return;
	}

	// check that it is in game or basegame dir
	if( strlen( filename ) < strlen( FS_GameDirectory() )+1 ||
		strncmp( filename, FS_GameDirectory(), strlen( FS_GameDirectory() ) ) ||
		filename[strlen( FS_GameDirectory() )] != '/' )
	{
		if( strlen( filename ) < strlen( FS_BaseGameDirectory() )+1 ||
			strncmp( filename, FS_BaseGameDirectory(), strlen( FS_BaseGameDirectory() ) ) ||
			filename[strlen( FS_BaseGameDirectory() )] != '/' )
		{
			Com_Printf( "Can't download, invalid game directory: %s\n", filename );
			CL_DownloadDone();
			return;
		}
	}

	if( FS_CheckPakExtension( filename ) )
	{
		if( strchr( strchr( filename, '/' ) + 1, '/' ) )
		{
			Com_Printf( "Refusing to download pack file to subdirectory: %s\n", filename );
			CL_DownloadDone();
			return;
		}

		if( !Q_strnicmp( COM_FileBase( filename ), "modules", strlen( "modules" ) ) )
		{
			if( !CL_CanDownloadModules() )
			{
				CL_DownloadDone();
				return;
			}
		}

		if( FS_FOpenBaseFile( filename, NULL, FS_READ ) != -1 )
		{
			Com_Printf( "Can't download, file already exists: %s\n", filename );
			CL_DownloadDone();
			return;
		}
	}
	else
	{
		if( strcmp( cls.download.requestname, strchr( filename, '/' ) + 1 ) )
		{
			Com_Printf( "Can't download, got different file than requested: %s\n", filename );
			CL_DownloadDone();
			return;
		}
	}

	if( cls.download.requestnext )
	{
		dl = cls.download.list;
		while( dl != NULL )
		{
			if( !Q_stricmp( dl->filename, filename ) )
			{
				Com_Printf( "Skipping, already tried downloading: %s\n", filename );
				CL_DownloadDone();
				return;
			}
			dl = dl->next;
		}
	}

	cls.download.name = ZoneCopyString( filename );

	alloc_size = strlen( filename ) + strlen( ".tmp" ) + 1;
	cls.download.tempname = Mem_ZoneMalloc( alloc_size );
	Q_snprintfz( cls.download.tempname, alloc_size, "%s.tmp", filename );

	cls.download.size = size;
	cls.download.checksum = checksum;
	cls.download.percent = 0;
	cls.download.timeout = 0;
	cls.download.retries = 0;
	cls.download.timestart = Sys_Milliseconds();
	cls.download.offset = 0;
	cls.download.baseoffset = 0;
	cls.download.pending_reconnect = qfalse;

	Cvar_ForceSet( "cl_download_name", COM_FileBase( cls.download.name ) );
	Cvar_ForceSet( "cl_download_percent", "0" );

	if( cls.download.requestnext )
	{
		dl = Mem_ZoneMalloc( sizeof( download_list_t ) );
		dl->filename = ZoneCopyString( cls.download.name );
		dl->next = cls.download.list;
		cls.download.list = dl;
	}

	if( cl_downloads_from_web->integer && url && url[0] != 0 )
	{
		char *fulltemp, *referer, *fullurl;
		qboolean success;
		int download_timeout;

		Com_Printf( "Web download: %s from %s/%s\n", cls.download.tempname, url, filename );

		alloc_size = strlen( FS_WriteDirectory() ) + 1 + strlen( cls.download.tempname ) + 1;
		fulltemp = Mem_ZoneMalloc( alloc_size );
		Q_snprintfz( fulltemp, alloc_size, "%s/%s", FS_WriteDirectory(), cls.download.tempname );

		alloc_size = strlen( APP_URI_SCHEME ) + strlen( NET_AddressToString( &cls.serveraddress ) ) + 1;
		referer = Mem_ZoneMalloc( alloc_size );
		Q_snprintfz( referer, alloc_size, APP_URI_SCHEME "%s", NET_AddressToString( &cls.serveraddress ) );
		Q_strlwr( referer );

		alloc_size = strlen( url ) + 1 + strlen( filename ) + 1;
		fullurl = Mem_ZoneMalloc( alloc_size );
		Q_snprintfz( fullurl, alloc_size, "%s/%s", url, filename );

		cls.download.web = qtrue;
		cls.download.disconnect = qfalse;

		download_timeout = cl_downloads_from_web_timeout->integer > 0 ? cl_downloads_from_web_timeout->integer : 0;
		success = Web_Get( fullurl, referer, fulltemp, qtrue, download_timeout, 30, CL_WebDownloadProgress, qfalse );

		cls.download.web = qfalse;

		Mem_ZoneFree( fullurl );
		Mem_ZoneFree( fulltemp );
		Mem_ZoneFree( referer );

		if( success )
		{
			Com_Printf( "Web download succesfull: %s\n", cls.download.tempname );

			CL_DownloadComplete();

			Mem_ZoneFree( cls.download.name );
			cls.download.name = NULL;
			Mem_ZoneFree( cls.download.tempname );
			cls.download.tempname = NULL;

			cls.download.size = 0;
			cls.download.checksum = 0;
			cls.download.percent = 0.0;

			Cvar_ForceSet( "cl_download_name", "" );
			Cvar_ForceSet( "cl_download_percent", "0" );
		}
		else
		{
			Com_Printf( "Web download of %s failed\n", cls.download.tempname );
		}

		// check if user pressed escape to stop the download
		if( cls.download.disconnect )
		{
			cls.download.disconnect = qfalse;
			CL_Disconnect_f();
			return;
		}

		if( success )
		{
			CL_DownloadDone();
			return;
		}
	}

	if( !allow_serverdownload )
	{
		CL_DownloadDone();
		return;
	}

	Com_Printf( "Server download: %s\n", cls.download.tempname );

	cls.download.baseoffset = cls.download.offset = FS_FOpenBaseFile( cls.download.tempname, &cls.download.filenum, FS_APPEND );
	if( cls.download.offset < 0 )
	{
		Com_Printf( "Can't download, couldn't open %s for writing\n", cls.download.tempname );

		Mem_ZoneFree( cls.download.name );
		cls.download.name = NULL;
		Mem_ZoneFree( cls.download.tempname );
		cls.download.tempname = NULL;

		cls.download.filenum = 0;
		cls.download.offset = 0;
		cls.download.size = 0;
		CL_DownloadDone();
		return;
	}

	// have to use Sys_Milliseconds because cls.realtime might be old from Web_Get
	cls.download.timeout = Sys_Milliseconds() + 3000;
	cls.download.retries = 0;

	CL_AddReliableCommand( va( "nextdl \"%s\" %i", cls.download.name, cls.download.offset ) );
}

/*
=====================
CL_StopServerDownload
=====================
*/
void CL_StopServerDownload( void )
{
	if( cls.download.filenum > 0 )
	{
		FS_FCloseFile( cls.download.filenum );
		cls.download.filenum = 0;
	}

	cls.download.filenum = 0;
	Mem_ZoneFree( cls.download.name );
	cls.download.name = NULL;
	Mem_ZoneFree( cls.download.tempname );
	cls.download.tempname = NULL;
	cls.download.offset = 0;
	cls.download.size = 0;
	cls.download.percent = 0.0;
	cls.download.timeout = 0;
	cls.download.retries = 0;

	Cvar_ForceSet( "cl_download_name", "" );
	Cvar_ForceSet( "cl_download_percent", "0" );
}

/*
=====================
CL_RetryDownload
Resends download request
Also aborts download if we have retried too many times
=====================
*/
static void CL_RetryDownload( void )
{
	if( ++cls.download.retries > 5 )
	{
		Com_Printf( "Download timed out: %s\n", cls.download.name );

		// let the server know we're done
		CL_AddReliableCommand( va( "nextdl \"%s\" %i", cls.download.name, -2 ) );
		CL_StopServerDownload();
		CL_DownloadDone();
	}
	else
	{
		cls.download.timeout = Sys_Milliseconds() + 3000;
		CL_AddReliableCommand( va( "nextdl \"%s\" %i", cls.download.name, cls.download.offset ) );
	}
}

/*
=====================
CL_CheckDownloadTimeout
Retry downloading if too much time has passed since last download packet was received
=====================
*/
void CL_CheckDownloadTimeout( void )
{
	if( !cls.download.timeout || cls.download.timeout > Sys_Milliseconds() )
		return;

	if( cls.download.filenum )
	{
		CL_RetryDownload();
	}
	else
	{
		Com_Printf( "Download request timed out.\n" );
		CL_DownloadDone();
	}
}

/*
=====================
CL_DownloadStatus_f
=====================
*/
void CL_DownloadStatus_f( void )
{
	if( !cls.download.requestname )
	{
		Com_Printf( "No download active\n" );
		return;
	}

	if( !cls.download.name )
	{
		Com_Printf( "%s: Requesting\n", COM_FileBase( cls.download.requestname ) );
		return;
	}

	Com_Printf( "%s: %s download %3.2f%c done\n", COM_FileBase( cls.download.name ),
		( cls.download.web ? "Web" : "Server" ), cls.download.percent * 100.0f, '%' );
}

/*
=====================
CL_DownloadCancel_f
=====================
*/
void CL_DownloadCancel_f( void )
{
	if( !cls.download.requestname )
	{
		Com_Printf( "No download active\n" );
		return;
	}

	if( !cls.download.name )
	{
		CL_DownloadDone();
		Com_Printf( "Canceled download request\n" );
		return;
	}

	if( cls.download.web )
	{
		cls.download.disconnect = qtrue; // FIXME
	}
	else
	{
		Com_Printf( "Canceled download of %s\n", cls.download.name );

		CL_AddReliableCommand( va( "nextdl \"%s\" %i", cls.download.name, -2 ) ); // let the server know we're done
		FS_RemoveBaseFile( cls.download.tempname );
		CL_StopServerDownload();
		CL_DownloadDone();
	}
}

/*
=====================
CL_ParseDownload
Handles download message from the server.
Writes data to the file and requests next download block.
=====================
*/
static void CL_ParseDownload( msg_t *msg )
{
	size_t size, offset;
	char *svFilename;

	// read the data
	svFilename = MSG_ReadString( msg );
	offset = MSG_ReadLong( msg );
	size = MSG_ReadLong( msg );

	if( size < 0 )
	{
		Com_Printf( "Error: Invalid size on a download message\n" );
		CL_RetryDownload();
		return;
	}

	if( msg->readcount + size > msg->cursize )
	{
		Com_Printf( "Error: Download message didn't have as much data as it promised\n" );
		CL_RetryDownload();
		return;
	}

	if( !cls.download.filenum )
	{
		Com_Printf( "Error: Download message while not dowloading\n" );
		msg->readcount += size;
		return;
	}

	if( Q_stricmp( cls.download.name, svFilename ) )
	{
		Com_Printf( "Error: Download message for wrong file\n" );
		msg->readcount += size;
		return;
	}

	if( offset+size > cls.download.size )
	{
		Com_Printf( "Error: Invalid download message\n" );
		msg->readcount += size;
		CL_RetryDownload();
		return;
	}

	if( cls.download.offset != offset )
	{
		Com_Printf( "Error: Download message for wrong position\n" );
		msg->readcount += size;
		CL_RetryDownload();
		return;
	}

	FS_Write( msg->data + msg->readcount, size, cls.download.filenum );
	msg->readcount += size;
	cls.download.offset += size;
	cls.download.percent = cls.download.offset * 1.0f / cls.download.size;

	Cvar_ForceSet( "cl_download_percent", va( "%.1f", cls.download.percent * 100 ) );

	if( cls.download.offset < cls.download.size )
	{
		cls.download.timeout = Sys_Milliseconds() + 3000;
		cls.download.retries = 0;

		CL_AddReliableCommand( va( "nextdl \"%s\" %i", cls.download.name, cls.download.offset ) );
	}
	else
	{
		Com_Printf( "Download complete: %s\n", cls.download.name );

		FS_FCloseFile( cls.download.filenum );
		cls.download.filenum = 0;

		CL_DownloadComplete();

		// let the server know we're done
		CL_AddReliableCommand( va( "nextdl \"%s\" %i", cls.download.name, -1 ) );

		CL_StopServerDownload();

		CL_DownloadDone();
	}
}

/*
=====================================================================

SERVER CONNECTING MESSAGES

=====================================================================
*/

static void CL_ParseConfigstringCommand( void );
/*
==================
CL_ParseServerData
==================
*/
static void CL_ParseServerData( msg_t *msg )
{
	const char *str, *gamedir;
	int i, sv_bitflags, numpure;
	qboolean game_restarted = qfalse;

	Com_DPrintf( "Serverdata packet received.\n" );

	// wipe the client_state_t struct

	CL_ClearState();
	CL_SetClientState( CA_CONNECTED );

	// parse protocol version number
	i = MSG_ReadLong( msg );

	if( i != APP_PROTOCOL_VERSION )
		Com_Error( ERR_DROP, "Server returned version %i, not %i", i, APP_PROTOCOL_VERSION );

	cl.servercount = MSG_ReadLong( msg );
	cl.snapFrameTime = (unsigned int)MSG_ReadShort( msg );

	// base game directory
	str = MSG_ReadString( msg );
	if( !str || !str[0] )
		Com_Error( ERR_DROP, "Server sent an empty base game directory" );
	if( !COM_ValidateRelativeFilename( str ) || strchr( str, '/' ) )
		Com_Error( ERR_DROP, "Server sent an invalid base game directory: %s", str );
	if( strcmp( FS_BaseGameDirectory(), str ) )
	{
		Com_Error( ERR_DROP, "Server has different base game directory (%s) than the client (%s)", str,
			FS_BaseGameDirectory() );
	}

	// game directory
	str = MSG_ReadString( msg );
	if( !str || !str[0] )
		Com_Error( ERR_DROP, "Server sent an empty game directory" );
	if( !COM_ValidateRelativeFilename( str ) || strchr( str, '/' ) )
		Com_Error( ERR_DROP, "Server sent an invalid game directory: %s", str );
	gamedir = FS_GameDirectory();
	if( strcmp( str, gamedir ) )
	{
		if( !FS_SetGameDirectory( str, qtrue ) )
			Com_Error( ERR_DROP, "Failed to load game directory set by server: %s", str );
		ML_Restart( qtrue );
		game_restarted = qtrue;
	}

	cls.sv_maxclients = MSG_ReadShort( msg );
	cl.playernum = MSG_ReadShort( msg );
	Q_strncpyz( cl.servermessage, MSG_ReadString( msg ), sizeof( cl.servermessage ) );
	sv_bitflags = MSG_ReadByte( msg );

	// pure list

	// clean old, if necessary
	Com_FreePureList( &cls.purelist );

	// add new
	numpure = MSG_ReadShort( msg );
	while( numpure > 0 )
	{
		const char *pakname = MSG_ReadString( msg );
		const unsigned checksum = MSG_ReadLong( msg );

		Com_AddPakToPureList( &cls.purelist, pakname, checksum, NULL );

		numpure--;
	}

	//assert( numpure == 0 );

	// get the configstrings request
	CL_AddReliableCommand( va( "configstrings %i 0", cl.servercount ) );

	// no need to restart media after a vid_restart forced by game directory change
	if( !game_restarted )
		CL_RestartMedia();

	cls.sv_pure = ( sv_bitflags & SV_BITFLAGS_PURE ) ? qtrue : qfalse;

	// separate the printfs so the server message can have a color
	Com_Printf( "\n%s\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n", S_COLOR_RED );
	Com_Printf( "%s%s\n\n", S_COLOR_WHITE, cl.servermessage );
}

//==================
//CL_ParseBaselines
//==================
static void CL_ParseBaselines( msg_t *msg )
{
	entity_state_t *from, *to;
	int entNum;
	unsigned int bits;

	assert( msg != NULL );

	memset( &nullEntityState, 0, sizeof( nullEntityState ) );

	entNum = Com_ReadEntityBits( msg, &bits );
	if( msg->readcount > msg->cursize )
	{                                 // something is wrong in the message
		Com_Printf( "WARNING CL_ParseBaselines: msg->readcount >= msg->cursize\n" );
		return;
	}
	if( entNum < 0 || entNum >= MAX_EDICTS )
	{                                      // something is wrong in the message
		Com_Printf( "WARNING CL_ParseBaselines: bad entNum %i\n", entNum );
		return;
	}

	from = &nullEntityState;
	to = cl.snapsData.baselines + entNum;
	Com_ReadDeltaEntityState( msg, from, to, entNum, bits );
}

//=================
//CL_ParseFrame - Parses the frame and stores the data in snap backups, but doesn't fire the new snap
//=================
static void CL_ParseFrame( msg_t *msg )
{
	snapshot_t *snap, *oldSnap;
	int delta;

	oldSnap = ( cl.receivedSnapNum > 0 ) ? &cl.snapShots[cl.receivedSnapNum & SNAPS_BACKUP_MASK] : NULL;

	snap = CL_ParseSnapshot( msg );

	if( snap->valid )
	{
		cl.receivedSnapNum = snap->snapNum;

		if( cls.demo.recording )
		{
			if( cls.demo.waiting && !snap->deltaSnapNum )
			{
				cls.demo.waiting = qfalse; // we can start recording now
				cls.demo.basetime = snap->timeStamp;
			}

			if( !cls.demo.waiting )
				cls.demo.duration = snap->timeStamp - cls.demo.basetime;
		}

		if( cl_debug_timeDelta->integer )
		{
			if( oldSnap != NULL && ( oldSnap->snapNum + 1 != snap->snapNum ) )
				Com_Printf( S_COLOR_RED"***** SnapShot lost\n" );
		}

		// the first snap, fill all the timeDeltas with the same value
		// don't let delta add big jumps to the smoothing ( a stable connection produces jumps inside +-3 range)
		delta = ( snap->timeStamp - cl.snapFrameTime ) - cls.gametime;
		if( cl.currentSnapNum <= 0 || delta < cl.newServerTimeDelta - 175 || delta > cl.newServerTimeDelta + 175 )
		{
			CL_RestartTimeDeltas( delta );
		}
		else
		{
			if( cl_debug_timeDelta->integer )
			{
				if( delta < cl.newServerTimeDelta - (int)cl.snapFrameTime )
					Com_Printf( S_COLOR_CYAN"***** timeDelta low clamp\n" );
				else if( delta > cl.newServerTimeDelta + (int)cl.snapFrameTime )
					Com_Printf( S_COLOR_CYAN"***** timeDelta high clamp\n" );
			}

			clamp( delta, cl.newServerTimeDelta - (int)cl.snapFrameTime, cl.newServerTimeDelta + (int)cl.snapFrameTime );

			cl.serverTimeDeltas[cl.receivedSnapNum & MASK_TIMEDELTAS_BACKUP] = delta;
		}
	}
}

//========= StringCommands================

/*
==================
CL_CmdStuffText_f
==================
*/
static void CL_CmdStuffText_f( void )
{
	char *s = Cmd_Argv( 1 );
	Com_DPrintf( "stufftext: %s\n", s );
	Cbuf_AddText( s );
}

/*
==================
CL_ValidateConfigstring
==================
*/
static qboolean CL_ValidateConfigstring( char *string )
{
	char *p;
	qboolean opened = qfalse;
	int parity = 0;

	if( !string || !string[0] )
		return qfalse;

	p = string;
	while( *p )
	{
		if( *p == '\"' )
		{
			if( opened )
			{
				parity--;
				opened = qfalse;
			}
			else
			{
				parity++;
				opened = qtrue;
			}
		}
		p++;
	}

	if( parity != 0 )
		return qfalse;

	return qtrue;
}

/*
* CL_CvarInfoRequest_f
*/
static void CL_CvarInfoRequest_f( void )
{
	char string[MAX_STRING_CHARS];
	char *cvarName;
	const char *cvarString;

	if( cls.demo.playing )
		return;

	if( Cmd_Argc() < 1 )
		return;

	cvarName = Cmd_Argv( 1 );

	string[0] = 0;
	Q_strncatz( string, "cvarinfo \"", sizeof( string ) );

	if( strlen( string ) + strlen( cvarName ) + 1 /*quote*/ + 1 /*space*/ >= MAX_STRING_CHARS - 1 )
	{
		CL_AddReliableCommand( "cvarinfo \"invalid\"" );
		return;
	}

	Q_strncatz( string, cvarName, sizeof( string ) );
	Q_strncatz( string, "\" ", sizeof( string ) );

	cvarString = Cvar_String( cvarName );
	if( !cvarString[0] )
		cvarString = "not found";

	if( strlen( string ) + strlen( cvarString ) + 2 /*quotes*/ >= MAX_STRING_CHARS - 1 )
	{
		if( strlen( string ) + strlen( " \"too long\"" ) < MAX_STRING_CHARS - 1 )
			CL_AddReliableCommand( va( "%s\"too long\"", string ) );
		else
			CL_AddReliableCommand( "cvarinfo \"invalid\"" );
			
		return;
	}

	Q_strncatz( string, "\"", sizeof( string ) );
	Q_strncatz( string, cvarString, sizeof( string ) );
	Q_strncatz( string, "\"", sizeof( string ) );

	CL_AddReliableCommand( string );
}

/*
==================
CL_ParseConfigstringCommand
==================
*/
static void CL_ParseConfigstringCommand( void )
{
	int i;
	char *s;

	if( Cmd_Argc() < 2 )
		return;

	i = atoi( Cmd_Argv( 1 ) );
	s = Cmd_Argv( 2 );

	if( !s || !s[0] )
		return;

	if( cl_debug_serverCmd->integer && ( cls.state >= CA_ACTIVE || cls.demo.playing ) )
		Com_Printf( "CL_ParseConfigstringCommand(%i): \"%s\"\n", i, s );

	if( i < 0 || i >= MAX_CONFIGSTRINGS )
		Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );

	// warn if configstring overflow
	if( strlen( s ) >= MAX_CONFIGSTRING_CHARS )
	{
		Com_Printf( "%sWARNING:%s Configstring %i overflowed\n", S_COLOR_YELLOW, S_COLOR_WHITE, i );
		Com_Printf( "%s%s\n", S_COLOR_WHITE, s );
	}

	if( !CL_ValidateConfigstring( s ) )
	{
		Com_Printf( "%sWARNING:%s Invalid Configstring (%i): %s\n", S_COLOR_YELLOW, S_COLOR_WHITE, i, s );
		return;
	}

	Q_strncpyz( cl.configstrings[i], s, sizeof( cl.configstrings[i] ) );

	// allow cgame to update it too
	CL_GameModule_ConfigStringUpdate( i );
}

typedef struct
{
	char *name;
	void ( *func )( void );
} svcmd_t;

svcmd_t svcmds[] =
{
	{ "forcereconnect", CL_Reconnect_f },
	{ "reconnect", CL_ServerReconnect_f },
	{ "changing", CL_Changing_f },
	{ "precache", CL_Precache_f },
	{ "cmd", CL_ForwardToServer_f },
	{ "stufftext", CL_CmdStuffText_f },
	{ "cs", CL_ParseConfigstringCommand },
	{ "disconnect", CL_ServerDisconnect_f },
	{ "initdownload", CL_InitDownload_f },
	{ "cvarinfo", CL_CvarInfoRequest_f },

	{ NULL, NULL }
};


/*
==================
CL_ParseServerCommand
==================
*/
static void CL_ParseServerCommand( msg_t *msg )
{
	const char *s;
	char *text;
	svcmd_t *cmd;

	text = MSG_ReadString( msg );
	Cmd_TokenizeString( text );
	s = Cmd_Argv( 0 );

	if( cl_debug_serverCmd->integer && ( cls.state < CA_ACTIVE || cls.demo.playing ) )
		Com_Printf( "CL_ParseServerCommand: \"%s\"\n", text );

	// filter out these server commands to be called from the client
	for( cmd = svcmds; cmd->name; cmd++ )
	{
		if( !strcmp( s, cmd->name ) )
		{
			cmd->func();
			return;
		}
	}

	CL_GameModule_ServerCommand();
}

static void CL_ParseServerCommands( msg_t *msg )
{
	unsigned int cmdNum;
	unsigned int cmdFirst = (unsigned)MSG_ReadLong( msg );
	unsigned int cmdCount = (unsigned)MSG_ReadShort( msg );

	for( cmdNum = cmdFirst; cmdNum < cmdFirst + cmdCount; cmdNum++ )
	{
		// in demos we accept cmdFirst to be 0 for non-reliable-checked commands
		if( cls.demo.playing && cmdFirst == 0 )
		{
			CL_ParseServerCommand( msg );
			continue;
		}
		else if( cmdNum == 0 )
			Com_Error( ERR_DISCONNECT, "CL_ParseServerMessage: Invalid cmdNum value received: %i\n", cmdNum );

		if( cmdNum > cls.lastExecutedServerCommand )
		{
			CL_ParseServerCommand( msg );
			cls.lastExecutedServerCommand = cmdNum;
		}
		else
			MSG_ReadString( msg ); // read and ignore
	}
}

/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

void SHOWNET( msg_t *msg, char *s )
{
	if( cl_shownet->integer >= 2 )
		Com_Printf( "%3i:%s\n", msg->readcount-1, s );
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage( msg_t *msg )
{
	int cmd;

	if( cl_shownet->integer == 1 )
	{
		Com_Printf( "%i ", msg->cursize );
	}
	else if( cl_shownet->integer >= 2 )
	{
		Com_Printf( "------------------\n" );
	}

	// parse the message
	while( 1 )
	{
		if( msg->readcount > msg->cursize )
		{
			Com_Error( ERR_DROP, "CL_ParseServerMessage: Bad server message" );
			break;
		}

		cmd = MSG_ReadByte( msg );
		if( cl_debug_serverCmd->integer & 4 )
		{
			if( cmd == -1 )
				Com_Printf( "%3i:CMD %i %s\n", msg->readcount-1, cmd, "EOF" );
			else
				Com_Printf( "%3i:CMD %i %s\n", msg->readcount-1, cmd, !svc_strings[cmd] ? "bad" : svc_strings[cmd] );
		}

		if( cmd == -1 )
		{
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if( cl_shownet->integer >= 2 )
		{
			if( !svc_strings[cmd] )
				Com_Printf( "%3i:BAD CMD %i\n", msg->readcount-1, cmd );
			else
				SHOWNET( msg, svc_strings[cmd] );
		}

		// other commands
		switch( cmd )
		{
		default:
			Com_Error( ERR_DROP, "CL_ParseServerMessage: Illegible server message" );
			break;

		case svc_nop:
			// Com_Printf( "svc_nop\n" );
			break;

		case svc_servercmd:
			CL_ParseServerCommands( msg );
			break;

		case svc_serverdata:
			{
				if( cls.state == CA_HANDSHAKE )
				{
					Cbuf_Execute(); // make sure any stuffed commands are done
					CL_ParseServerData( msg );
				}
				else
				{
					return; // ignore rest of the packet (serverdata is always sent alone)
				}
			}
			break;

		case svc_spawnbaseline:
			CL_ParseBaselines( msg );
			break;

		case svc_download:
			CL_ParseDownload( msg );
			break;

		case svc_clcack:
			cls.reliableAcknowledge = (unsigned)MSG_ReadLong( msg );
			cls.ucmdAcknowledged = (unsigned)MSG_ReadLong( msg );
			if( cl_debug_serverCmd->integer & 4 )
				Com_Printf( "svc_clcack:reliable cmd ack:%i ucmdack:%i\n", cls.reliableAcknowledge, cls.ucmdAcknowledged );
			break;

		case svc_frame:
			CL_ParseFrame( msg );
			break;

		case svc_demoinfo:
			assert( cls.demo.playing );
			cls.demo.basetime = (unsigned)MSG_ReadLong( msg );
			cls.demo.duration = (unsigned)MSG_ReadLong( msg );
			MSG_ReadLong( msg );
			break;

		case svc_gameinfo:
		case svc_playerinfo:
		case svc_packetentities:
			Com_Error( ERR_DROP, "Out of place frame data" );
			break;

		case svc_extension:
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

	CL_AddNetgraph();

	//
	// if recording demos, copy the message out
	//
	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if( cls.demo.recording && !cls.demo.waiting )
		CL_WriteDemoMessage( msg );
}
