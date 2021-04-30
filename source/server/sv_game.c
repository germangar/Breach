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
// sv_game.c -- interface to the game dll

#include "server.h"

game_export_t *ge;

EXTERN_API_FUNC void *GetGameAPI( void * );

mempool_t *sv_gameprogspool;
static void *module_handle;

/*
   ===============
   PF_Layout

   Sends the layout to a single client
   ===============
 */
static void PF_Layout( int clientNum, char *string )
{
	client_t *client;

	if( !string || !string[0] )
		return;

	if( clientNum < 0 || clientNum >= sv_maxclients->integer )
		return;

	client = svs.clients + clientNum;
	if( client->fakeClient )
		return;
	SV_SendServerCommand( client, "layout \"%s\"", string );
}

/*
   ===============
   PF_DropClient
   ===============
 */
static void PF_DropClient( int clientNum, int type, char *message )
{
	if( clientNum < 0 || clientNum >= sv_maxclients->integer )
		return;

	SV_DropClient( &svs.clients[clientNum], type, "%s", message );
}

//===============
//PF_GetClientState
// Game code asks for the state of this client
//===============
static int PF_GetClientState( int numClient )
{
	if( numClient < 0 || numClient >= sv_maxclients->integer )
		Com_Error( ERR_DROP, "PF_GetClientState: Bad client number" );
	return svs.clients[numClient].state;
}

/*
   ===============
   PF_ServerCmd

   Sends the server command to clients.
   if ent is NULL the command will be sent to all connected clients
   ===============
 */
static void PF_ServerCmd( int clientNum, char *cmd )
{
	client_t *client;

	client = ( clientNum < 0 || clientNum >= sv_maxclients->integer ) ? NULL : &svs.clients[ clientNum ];
	SV_SendServerCommand( client, cmd );
}

/*
   ===============
   PF_dprint

   Debug print to server console
   ===============
 */
static void PF_dprint( const char *msg )
{
	int i;
	char copy[MAX_PRINTMSG];

	if( !msg )
		return;

	// mask off high bits and colored strings
	for( i = 0; i < sizeof( copy )-1 && msg[i]; i++ )
		copy[i] = msg[i]&127;
	copy[i] = 0;

	Com_Printf( "%s", copy );
}

/*
   ===============
   PF_error

   Abort the server with a game error
   ===============
 */
static void PF_error( const char *msg )
{
	int i;
	char copy[MAX_PRINTMSG];

	if( !msg )
	{
		Com_Error( ERR_DROP, "Game Error: unknown error" );
		return;
	}

	// mask off high bits and colored strings
	for( i = 0; i < sizeof( copy )-1 && msg[i]; i++ )
		copy[i] = msg[i]&127;
	copy[i] = 0;

	Com_Error( ERR_DROP, "Game Error: %s", copy );
}

/*
   ===============
   PF_StuffCmd
   ===============
 */
static void PF_StuffCmd( int clientNum, char *string )
{
	client_t *client;

	if( !string || !string[0] )
		return;

	if( clientNum < 0 || clientNum >= sv_maxclients->integer )
		return;

	client = svs.clients + clientNum;
	if( client->fakeClient )
		return;
	SV_SendServerCommand( client, "stufftext \"%s\"", string );
}

/*
   ===============
   PF_Configstring
   ===============
 */
static void PF_Configstring( int index, const char *val )
{
	char cstring[MAX_CONFIGSTRING_CHARS];

	if( !val )
		return;

	if( index < 0 || index >= MAX_CONFIGSTRINGS )
		Com_Error( ERR_DROP, "configstring: bad index %i", index );

	if( strlen( val ) >= sizeof( sv.configstrings[0] ) )
	{
		Com_Printf( "WARNING:'PF_Configstring', configstring %i was overflowing\n", index );
	}

	Q_strncpyz( cstring, val, sizeof( cstring ) );

	if( !COM_ValidateConfigstring( cstring ) )
	{
		Com_Printf( "WARNING: 'PF_Configstring' invalid configstring %i: %s\n", index, cstring );
		return;
	}

	// ignore if no changes
	if( !strcmp( sv.configstrings[index], cstring ) )
		return;

	// change the string in sv
	Q_strncpyz( sv.configstrings[index], cstring, sizeof( sv.configstrings[index] ) );

	if( sv.state != ss_loading )
		SV_SendServerCommand( NULL, "cs %i \"%s\"", index, sv.configstrings[index] );
}

/*
* PF_GetConfigstring
*/
static const char *PF_GetConfigstring( int num )
{
	if( num >= 0 && num < MAX_CONFIGSTRINGS )
		return sv.configstrings[num];

	return NULL;
}

/*
* PF_PureSound
*/
static void PF_PureSound( const char *name )
{
	const char *extension;
	char tempname[MAX_CONFIGSTRING_CHARS];

	if( sv.state != ss_loading )
		return;

	if( !name || !name[0] || strlen( name ) >= MAX_CONFIGSTRING_CHARS )
		return;

	Q_strncpyz( tempname, name, sizeof( tempname ) );

	if( !COM_FileExtension( tempname ) )
	{
		extension = FS_FirstExtension( tempname, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS );
		if( !extension )
			return;

		COM_ReplaceExtension( tempname, extension, sizeof( tempname ) );
	}

	SV_AddPureFile( tempname );
}

/*
* SV_AddPureShader
*
* FIXME: For now we don't parse shaders, but simply assume that it uses the same name .tga or .jpg
*/
static void SV_AddPureShader( const char *name )
{
	const char *extension;
	char tempname[MAX_CONFIGSTRING_CHARS];

	if( !name || !name[0] )
		return;

	assert( name && name[0] && strlen( name ) < MAX_CONFIGSTRING_CHARS );

	if( !Q_strnicmp( name, "textures/common/", strlen( "textures/common/" ) ) )
		return;

	Q_strncpyz( tempname, name, sizeof( tempname ) );

	if( !COM_FileExtension( tempname ) )
	{
		extension = FS_FirstExtension( tempname, IMAGE_EXTENSIONS, NUM_IMAGE_EXTENSIONS );
		if( !extension )
			return;

		COM_ReplaceExtension( tempname, extension, sizeof( tempname ) );
	}

	SV_AddPureFile( tempname );
}

/*
* SV_AddPureBSP
*/
static void SV_AddPureBSP( void )
{
	int i;
	const char *shader;

	SV_AddPureFile( sv.configstrings[CS_WORLDMODEL] );
	for( i = 0; ( shader = CM_ShaderrefName( svs.cms, i ) ); i++ )
		SV_AddPureShader( shader );
}

/*
* PF_PureModel
*/
static void PF_PureModel( const char *name )
{
	if( sv.state != ss_loading )
		return;

	if( !name || !name[0] || strlen( name ) >= MAX_CONFIGSTRING_CHARS )
		return;

	if( name[0] == '*' )
	{                       // inline model
		if( !strcmp( name, "*0" ) )
			SV_AddPureBSP(); // world
	}
	else
	{
		SV_AddPureFile( name );
	}
}

/*
   =================
   PF_inPVS

   Also checks portalareas so that doors block sight
   =================
 */
static qboolean PF_inPVS( vec3_t p1, vec3_t p2 )
{
	int leafnum;
	int cluster;
	int area1, area2;
	qbyte *mask;

	leafnum = CM_PointLeafnum( svs.cms, p1 );
	cluster = CM_LeafCluster( svs.cms, leafnum );
	area1 = CM_LeafArea( svs.cms, leafnum );
	mask = CM_ClusterPVS( svs.cms, cluster );

	leafnum = CM_PointLeafnum( svs.cms, p2 );
	cluster = CM_LeafCluster( svs.cms, leafnum );
	area2 = CM_LeafArea( svs.cms, leafnum );
	if( mask && ( !( mask[cluster>>3] & ( 1<<( cluster&7 ) ) ) ) )
		return qfalse;
	if( !CM_AreasConnected( svs.cms, area1, area2 ) )
		return qfalse; // a door blocks sight
	return qtrue;
}

/*
===============
PF_MemAlloc
===============
*/
static void *PF_MemAlloc( size_t size, const char *filename, int fileline )
{
	return _Mem_Alloc( sv_gameprogspool, size, MEMPOOL_GAMEPROGS, 0, filename, fileline );
}

/*
===============
PF_MemFree
===============
*/
static void PF_MemFree( void *data, const char *filename, int fileline )
{
	_Mem_Free( data, MEMPOOL_GAMEPROGS, 0, filename, fileline );
}

//==============================================

static inline int PF_CM_NumInlineModels( void ) 
{
	return CM_NumInlineModels( svs.cms );
}

static struct cmodel_s	*PF_CM_InlineModel( int num )
{
	return CM_InlineModel( svs.cms, num );
}

static int PF_CM_TransformedPointContents( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles )
{
	return CM_TransformedPointContents( svs.cms, p, cmodel, origin, angles );
}

static void PF_CM_TransformedBoxTrace( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles )
{
	CM_TransformedBoxTrace( svs.cms, tr, start, end, mins, maxs, cmodel, brushmask, origin, angles );
}

static void PF_CM_InlineModelBounds( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs )
{
	CM_InlineModelBounds( svs.cms, cmodel, mins, maxs );
}

static struct cmodel_s *PF_CM_ModelForBBox( vec3_t mins, vec3_t maxs )
{
	return CM_ModelForBBox( svs.cms, mins, maxs );
}

static qboolean PF_CM_AreasConnected ( int area1, int area2 )
{
	return CM_AreasConnected( svs.cms, area1, area2 );
}

static void PF_CM_SetAreaPortalState( int area, int otherarea, qboolean open )
{
	CM_SetAreaPortalState( svs.cms, area, otherarea, open );
}

static int PF_CM_BoxLeafnums( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode )
{
	return CM_BoxLeafnums( svs.cms, mins, maxs, list, listsize, topnode );
}

static int PF_CM_LeafCluster( int leafnum )
{
	return CM_LeafCluster( svs.cms, leafnum );
}

static int PF_CM_LeafArea( int leafnum )
{
	return CM_LeafArea( svs.cms, leafnum );
}

/*
===============
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_ShutdownGameProgs( void )
{
	if( !ge )
		return;

	ge->Shutdown();
	Mem_FreePool( &sv_gameprogspool );
	Com_UnloadGameLibrary( &module_handle );
	ge = NULL;
}

/*
   ===============
   SV_InitGameProgs

   Init the game subsystem for a new map
   ===============
 */
void SV_InitGameProgs( void )
{
	int apiversion;
	game_import_t import;
	void *( *builtinAPIfunc )(void *) = NULL;
	char manifest[MAX_INFO_STRING];
#ifdef GAME_HARD_LINKED
	builtinAPIfunc = GetGameAPI;
#endif

	// unload anything we have now
	if( ge )
		SV_ShutdownGameProgs();

	sv_gameprogspool = _Mem_AllocPool( NULL, "Game Progs", MEMPOOL_GAMEPROGS, __FILE__, __LINE__ );

	// load a new game dll
	import.Print = PF_dprint;
	import.Error = PF_error;
	import.ServerCmd = PF_ServerCmd;

	import.SetVIS = SV_SetVIS;
	import.PVSCullEntity = SV_PVSCullEntity;
	import.PHSCullEntity = SV_PHSCullEntity;
	import.AreaCullEntity = SV_AreaCullEntity;

	import.inPVS = PF_inPVS;

	import.CM_NumInlineModels = PF_CM_NumInlineModels;
	import.CM_InlineModel = PF_CM_InlineModel;
	import.CM_TransformedPointContents = PF_CM_TransformedPointContents;
	import.CM_TransformedBoxTrace = PF_CM_TransformedBoxTrace;
	import.CM_InlineModelBounds = PF_CM_InlineModelBounds;
	import.CM_ModelForBBox = PF_CM_ModelForBBox;
	import.CM_AreasConnected = PF_CM_AreasConnected;
	import.CM_SetAreaPortalState = PF_CM_SetAreaPortalState;
	import.CM_BoxLeafnums = PF_CM_BoxLeafnums;
	import.CM_LeafCluster = PF_CM_LeafCluster;
	import.CM_LeafArea = PF_CM_LeafArea;

	import.Milliseconds = Sys_Milliseconds;

	import.ConfigString = PF_Configstring;
	import.GetConfigString = PF_GetConfigstring;
	import.Layout = PF_Layout;
	import.StuffCmd = PF_StuffCmd;

	import.ModelIndex = SV_ModelIndex;
	import.SoundIndex = SV_SoundIndex;
	import.ImageIndex = SV_ImageIndex;
	import.SkinIndex = SV_SkinIndex;

	import.PureSound = PF_PureSound;
	import.PureModel = PF_PureModel;

	import.FS_FOpenFile = FS_FOpenFile;
	import.FS_Read = FS_Read;
	import.FS_Write = FS_Write;
	import.FS_Print = FS_Print;
	import.FS_Tell = FS_Tell;
	import.FS_Seek = FS_Seek;
	import.FS_Eof = FS_Eof;
	import.FS_Flush = FS_Flush;
	import.FS_FCloseFile = FS_FCloseFile;
	import.FS_RemoveFile = FS_RemoveFile;
	import.FS_GetFileList = FS_GetFileList;
	import.FS_FirstExtension = FS_FirstExtension;

	import.ML_Update = ML_Update;
	import.ML_GetMapByNum = ML_GetMapByNum;
	import.ML_FilenameExists = ML_FilenameExists;

	import.Cmd_ExecuteText = Cbuf_ExecuteText;
	import.Cbuf_Execute = Cbuf_Execute;

	import.Mem_Alloc = PF_MemAlloc;
	import.Mem_Free = PF_MemFree;

	import.Cvar_Get = Cvar_Get;
	import.Cvar_Set = Cvar_Set;
	import.Cvar_SetValue = Cvar_SetValue;
	import.Cvar_ForceSet = Cvar_ForceSet;
	import.Cvar_Value = Cvar_Value;
	import.Cvar_String = Cvar_String;

	import.Cmd_Argc = Cmd_Argc;
	import.Cmd_Argv = Cmd_Argv;
	import.Cmd_Args = Cmd_Args;
	import.Cmd_AddCommand = Cmd_AddCommand;
	import.Cmd_RemoveCommand = Cmd_RemoveCommand;
	import.AddCommandString = Cbuf_AddText;

	import.FakeClientConnect = SVC_FakeConnect;
	import.DropClient = PF_DropClient;
	import.GetClientState = PF_GetClientState;
	import.ExecuteClientThinks = SV_ExecuteClientThinks;
	import.asGetAngelExport = Com_asGetAngelExport;

	// clear module manifest string
	assert( sizeof( manifest ) >= MAX_INFO_STRING );
	memset( manifest, 0, sizeof( manifest ) );

	ge = (game_export_t *)Com_LoadGameLibrary( "game", "GetGameAPI", &module_handle, &import, builtinAPIfunc, qfalse, manifest );
	if( !ge )
		Com_Error( ERR_DROP, "Failed to load game DLL" );

	apiversion = ge->API();
	if( apiversion != GAME_API_VERSION )
	{
		Com_UnloadGameLibrary( &module_handle );
		Mem_FreePool( &sv_gameprogspool );
		ge = NULL;
		Com_Error( ERR_DROP, "Game is version %i, not %i", apiversion, GAME_API_VERSION );
	}

	Cvar_ForceSet( "sv_modmanifest", manifest );

	// jalfixme: implement server protected configstrings
	Q_strncpyz( sv.configstrings[CS_MODMANIFEST], Cvar_String( "sv_modmanifest" ), sizeof( sv.configstrings[0] ) );

	// send maxclients and frametime to game module
	ge->Init( time( NULL ), sv_maxclients->integer, svc.snapFrameTime, APP_PROTOCOL_VERSION );
}
