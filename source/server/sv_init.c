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
#include "../qcommon/library.h"      // Sys_Library_Extension

server_constant_t svc;              // constant server info (trully persistant since sv_init)
server_static_t	svs;                // persistant server info
server_t sv;                 // local server

//================
//SV_FindIndex
//================
static int SV_FindIndex( const char *name, int start, int max, qboolean create )
{
	int i;

	if( !name || !name[0] )
		return 0;

	if( strlen( name ) >= MAX_CONFIGSTRING_CHARS )
		Com_Error( ERR_DROP, "Configstring too long: %s\n", name );

	for( i = 1; i < max && sv.configstrings[start+i][0]; i++ )
	{
		if( !strncmp( sv.configstrings[start+i], name, sizeof( sv.configstrings[start+i] ) ) )
			return i;
	}

	if( !create )
		return 0;

	if( i == max )
		Com_Error( ERR_DROP, "*Index: overflow" );

	Q_strncpyz( sv.configstrings[start+i], name, sizeof( sv.configstrings[i] ) );

	if( sv.state != ss_loading )
	{ // send the update to everyone
		SV_SendServerCommand( NULL, "cs %i \"%s\"", start+i, name );
	}

	return i;
}


int SV_ModelIndex( const char *name )
{
	return SV_FindIndex( name, CS_MODELS, MAX_MODELS, qtrue );
}

int SV_SoundIndex( const char *name )
{
	return SV_FindIndex( name, CS_SOUNDS, MAX_SOUNDS, qtrue );
}

int SV_ImageIndex( const char *name )
{
	return SV_FindIndex( name, CS_IMAGES, MAX_IMAGES, qtrue );
}

int SV_SkinIndex( const char *name )
{
	return SV_FindIndex( name, CS_SKINFILES, MAX_SKINFILES, qtrue );
}

//=================
//SV_PureList_f
//=================
void SV_PureList_f( void )
{
	purelist_t *purefile;

	Com_Printf( "Pure files:\n" );
	purefile = svs.purelist;
	while( purefile )
	{
		Com_Printf( "- %s (%u)\n", purefile->filename, purefile->checksum );
		purefile = purefile->next;
	}
}

//=================
//SV_AddPurePak
//=================
static void SV_AddPurePak( const char *pakname )
{
	if( !Com_FindPakInPureList( svs.purelist, pakname ) )
		Com_AddPakToPureList( &svs.purelist, pakname, FS_ChecksumBaseFile( pakname ), NULL );
}

//=================
//SV_AddPureFile
//=================
void SV_AddPureFile( const char *filename )
{
	const char *pakname;

	if( !filename || !strlen( filename ) )
		return;

	pakname = FS_PakNameForFile( filename );

	if( pakname )
	{
		Com_DPrintf( "Pure file: %s (%s)\n", pakname, filename );
		SV_AddPurePak( pakname );
	}
}

//=================
//SV_ReloadPureList
//=================
static void SV_ReloadPureList( void )
{
	char **paks;
	int i, numpaks;

	Com_FreePureList( &svs.purelist );

	// game modules
	if( sv_pure_forcemodulepk3->string[0] )
	{
		if( Q_strnicmp( COM_FileBase( sv_pure_forcemodulepk3->string ), "modules", strlen( "modules" ) ) ||
			!FS_IsPakValid( sv_pure_forcemodulepk3->string, NULL ) )
		{
			Com_Printf( "Warning: Invalid value for sv_pure_forcemodulepk3, disabling\n" );
			Cvar_ForceSet( "sv_pure_forcemodulepk3", "" );
		}
		else
		{
			SV_AddPurePak( sv_pure_forcemodulepk3->string );
		}
	}

	if( !sv_pure_forcemodulepk3->string[0] )
	{
		char *libname;
		int libname_size;

		libname_size = 5 + strlen( ARCH ) + strlen( LIB_SUFFIX ) + 1;
		libname = Mem_TempMalloc( libname_size );
		Q_snprintfz( libname, libname_size, "game_" ARCH LIB_SUFFIX );

		if( !FS_PakNameForFile( libname ) )
		{
			if( sv_pure->integer )
			{
				Com_Printf( "Warning: Game module not in pk3, disabling pure mode\n" );
				Com_Printf( "sv_pure_forcemodulepk3 can be used to force the pure system to use a different module\n" );
				Cvar_ForceSet( "sv_pure", "0" );
			}
		}
		else
		{
			SV_AddPureFile( libname );
		}

		Mem_TempFree( libname );
		libname = NULL;
	}

	// *pure.(pk3|pak)
	paks = NULL;
	numpaks = FS_GetExplicitPurePakList( &paks );
	if( numpaks )
	{
		for( i = 0; i < numpaks; i++ )
		{
			SV_AddPurePak( paks[i] );
			Mem_ZoneFree( paks[i] );
		}
		Mem_ZoneFree( paks );
	}
}

//================
//SV_SpawnServer
//Change the server to a new map, taking all connected clients along with it.
//================
static void SV_SpawnServer( const char *server, qboolean devmap )
{
	unsigned int checksum;
	int i;

	if( devmap )
		Cvar_ForceSet( "sv_cheats", "1" );
	Cvar_FixCheatVars();

	Com_Printf( "------- Server Initialization -------\n" );
	Com_DPrintf( "SpawnServer: %s\n", server );

	svs.spawncount++;   // any partially connected client will be restarted

	Com_SetServerState( ss_dead );

	// wipe the entire per-level structure
	memset( &sv, 0, sizeof( sv ) );
	svs.realtime = 0;
	svs.gametime = 0;
	// start sv.framenum at 1, cause 0 is reserved for nodelta requests
	sv.framenum = 1;
	SV_UpdateActivity();

	Q_strncpyz( sv.mapname, server, sizeof( sv.mapname ) );

	sv.nextSnapTime = 1000;

	// load the map
	Q_snprintfz( sv.configstrings[CS_WORLDMODEL], sizeof( sv.configstrings[CS_WORLDMODEL] ), "maps/%s.bsp", server );
	CM_LoadMap( svs.cms, sv.configstrings[CS_WORLDMODEL], qfalse, &checksum );

	Q_snprintfz( sv.configstrings[CS_MAPCHECKSUM], sizeof( sv.configstrings[CS_MAPCHECKSUM] ), "%i", checksum );

	// reserve the first modelIndexes for inline models
	for( i = 1; i < CM_NumInlineModels( svs.cms ); i++ )
		Q_snprintfz( sv.configstrings[CS_MODELS + i], sizeof( sv.configstrings[CS_MODELS + i] ), "*%i", i );

	// set serverinfo variable
	Cvar_FullSet( "mapname", sv.mapname, CVAR_SERVERINFO | CVAR_READONLY, qtrue );

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;
	Com_SetServerState( sv.state );

	// set purelist
	SV_ReloadPureList();

	// load and spawn all other entities
	ge->InitLevel( sv.mapname, CM_EntityString( svs.cms ), CM_EntityStringLen( svs.cms ), svs.gametime );

	// run two frames to allow everything to settle
	ge->RunFrame( svc.snapFrameTime, svs.gametime );
	ge->RunFrame( svc.snapFrameTime, svs.gametime );
	ge->SnapFrame( sv.framenum ); // set up the first snap so baselines start from a valid point
	SV_CreateBaselines();

	// all precaches are complete
	sv.state = ss_game;
	Com_SetServerState( sv.state );

	Com_Printf( "-------------------------------------\n" );
}

//==============
//SV_InitGame
//A brand new game has been started
//==============
void SV_InitGame( void )
{
	if( svs.initialized )
	{
		// cause any connected clients to reconnect
		SV_Shutdown( "Server restarted\n", qtrue );

		// SV_Shutdown will also call Cvar_GetLatchedVars
	}
	else
	{
		// make sure the client is down
		CL_Disconnect( NULL );
		SCR_BeginLoadingPlaque();

		// get any latched variable changes (sv_maxclients, etc)
		Cvar_GetLatchedVars( CVAR_LATCH );
	}

	svs.initialized = qtrue;

	// init clients
	if( sv_maxclients->integer <= 1 )
		Cvar_FullSet( "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH, qtrue );
	else if( sv_maxclients->integer > MAX_CLIENTS )
		Cvar_FullSet( "sv_maxclients", va( "%i", MAX_CLIENTS ), CVAR_SERVERINFO | CVAR_LATCH, qtrue );

	svs.spawncount = rand();
	svs.clients = Mem_Alloc( sv_mempool, sizeof( client_t )*sv_maxclients->integer );

	// init network stuff
	NET_Config( ( sv_maxclients->integer > 1 ) );

	// init game
	SV_InitGameProgs();

	// load the map
	assert( !svs.cms );
	svs.cms = CM_New( NULL );
}

//======================
//SV_Map
// command from the console or progs.
//======================
void SV_Map( const char *level, qboolean devmap )
{
	client_t *cl;
	int i;

	// skip the end-of-unit flag if necessary
	if( level[0] == '*' )
		level++;

	if( sv.state == ss_dead )
		SV_InitGame(); // the game is just starting

	// remove all bots before changing map
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state && cl->fakeClient )
		{
			SV_DropClient( cl, DROP_TYPE_GENERAL, "SV_Map" );
		}
	}

	// this used to be at SV_SpawnServer, but we need to do it before sending changing
	// so we don't send frames after sending changing command leave slots at start for clients only
	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		// needs to reconnect
		if( svs.clients[i].state > CS_CONNECTING )
			svs.clients[i].state = CS_CONNECTING;
		svs.clients[i].snapAcknowledged = 0;
	}

	SCR_BeginLoadingPlaque(); // for local system
	SV_BroadcastCommand( "changing\n" );
	SV_SendClientMessages();
	SV_SpawnServer( level, devmap );

	SV_BroadcastCommand( "reconnect\n" );
}
