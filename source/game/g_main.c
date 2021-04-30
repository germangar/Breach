/*
 */

#include "g_local.h"

game_locals_t game;
level_locals_t level;

struct gentity_s *worldEntity;

int meansOfDeath;

cvar_t *password;
cvar_t *dedicated;
cvar_t *developer;
cvar_t *filterban;
cvar_t *sv_cheats;

cvar_t *g_minculldistance;

cvar_t *g_antilag;
cvar_t *g_antilag_maxtimedelta;
cvar_t *g_antilag_timenudge;


/*
* G_API
*/
int G_API( void )
{
	return GAME_API_VERSION;
}

void G_DrawBox_NULL( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t angles, vec4_t color ) 
{
};

/*
* G_InitGameShared
* give gameshared access to some utilities
*/
static void G_InitGameShared( unsigned int maxclients, unsigned int snapFrameTime, int protocol )
{
	gs_moduleapi_t module;

	module.type = GS_MODULE_GAME;
	module.Malloc = trap_MemAlloc;
	module.Free = trap_MemFree;
	module.trap_Print = trap_Print;
	module.trap_Error = trap_Error;

	// file system
	module.trap_FS_FOpenFile = trap_FS_FOpenFile;
	module.trap_FS_Read = trap_FS_Read;
	module.trap_FS_FCloseFile = trap_FS_FCloseFile;

	// collision traps
	module.trap_CM_NumInlineModels = trap_CM_NumInlineModels;
	module.trap_CM_TransformedPointContents = trap_CM_TransformedPointContents;
	module.trap_CM_InlineModel = trap_CM_InlineModel;
	module.trap_CM_ModelForBBox = trap_CM_ModelForBBox;
	module.trap_CM_InlineModelBounds = trap_CM_InlineModelBounds;
	module.trap_CM_TransformedBoxTrace = trap_CM_TransformedBoxTrace;
	module.trap_CM_BoxLeafnums = trap_CM_BoxLeafnums;
	module.trap_CM_LeafCluster = trap_CM_LeafCluster;

	// the plan is to make this one inside gameshared later on
	module.GetClipStateForDeltaTime = GClip_GetClipStateForDeltaTime;

	module.predictedEvent = G_PredictedEvent;
	module.DrawBox = G_DrawBox_NULL;

	GS_Init( &module, maxclients, snapFrameTime, protocol );
}

/*
* G_Init
* This will be called when the dll is first loaded, which
* only happens when a new game is started.
*/
void G_Init( unsigned int seed, unsigned int maxclients, unsigned int snapFrameTime, int protocol )
{
	int i;
	srand( seed );

	G_InitGameShared( maxclients, snapFrameTime, protocol );

	GS_Printf( "==== Initializing Game Module ====\n" );

	memset( &game, 0, sizeof( game ) );
	game.framemsecs = snapFrameTime;

	trap_Cvar_Get( "gamename", trap_Cvar_String( "gamename" ), CVAR_SERVERINFO );
	trap_Cvar_Get( "gamedate", __DATE__, CVAR_SERVERINFO | CVAR_LATCH );

	developer = trap_Cvar_Get( "developer", "0", 0 );

	// noset vars
	dedicated = trap_Cvar_Get( "dedicated", "0", CVAR_NOSET );

	// latched vars
	sv_cheats = trap_Cvar_Get( "sv_cheats", "0", CVAR_SERVERINFO|CVAR_LATCH );

	password = trap_Cvar_Get( "password", "", CVAR_USERINFO );
	filterban = trap_Cvar_Get( "filterban", "1", 0 );

	// generic settings
	g_minculldistance = trap_Cvar_Get( "g_minculldistance", "0", CVAR_DEVELOPER|CVAR_LATCH );

	g_antilag = trap_Cvar_Get( "g_antilag", "1", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_LATCH );
	g_antilag_maxtimedelta = trap_Cvar_Get( "g_antilag_maxtimedelta", "200", CVAR_ARCHIVE );
	g_antilag_maxtimedelta->modified = qtrue;
	g_antilag_timenudge = trap_Cvar_Get( "g_antilag_timenudge", "0", CVAR_ARCHIVE );
	g_antilag_timenudge->modified = qtrue;

	// server commands
	G_AddCommands();

	// initialize all entities for this game
	game.entities = ( gentity_t * )G_Malloc( MAX_EDICTS * sizeof( game.entities[0] ) );
	memset( game.entities, 0, sizeof( game.entities ) );
	worldEntity = game.entities + ENTITY_WORLD;

	// initialize all clients for this game
	game.clients = ( gclient_t * )G_Malloc( gs.maxclients * sizeof( game.clients[0] ) );
	memset( game.clients, 0, sizeof( game.clients ) );

	game.numentities = gs.maxclients;

	// set client fields on player entities
	for( i = 0; i < gs.maxclients; i++ )
		game.entities[i].client = game.clients + i;
}

/*
* G_Shutdown
*/
void G_Shutdown( void )
{
	GS_Printf( "==== G_Shutdown ====\n" );

	GS_FreeSpacePartition();
	G_RemoveCommands();
	G_LevelFreePool();
}

//======================================================================

/*
* G_AllowDownload - gives permission for a specified file even if the server downloads are disabled.
*/
qboolean G_AllowDownload( int clientNum, const char *requestname, const char *uploadname )
{
	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "G_AllowDownload: Bad client number" );

	return qfalse;
}

//======================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	GS_Error( "%s", msg );
}

void Com_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	GS_Printf( "%s", msg );
}
#endif
