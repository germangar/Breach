/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

/*
* G_PrecacheLightStyles
* 0 normal
* 1 FLICKER 1
* 2 SLOW STRONG PULSE
* 3 CANDLE 1
* 4 FAST STROBE
* 5 GENTLE PULSE 1
* 6 FLICKER 2
* 7 CANDLE 2
* 8 CANDLE 3
* 9 SLOW STROBE
* 10 FLUORESCENT FLICKER
* 11 SLOW PULSE NOT FADE TO BLACK
*/
static void G_PrecacheLightStyles( void )
{
	trap_ConfigString( CS_LIGHTS + 0, "m" );
	trap_ConfigString( CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo" );
	trap_ConfigString( CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba" );
	trap_ConfigString( CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg" );
	trap_ConfigString( CS_LIGHTS + 4, "mamamamamama" );
	trap_ConfigString( CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj" );
	trap_ConfigString( CS_LIGHTS + 6, "nmonqnmomnmomomno" );
	trap_ConfigString( CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm" );
	trap_ConfigString( CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa" );
	trap_ConfigString( CS_LIGHTS + 9, "aaaaaaaazzzzzzzz" );
	trap_ConfigString( CS_LIGHTS + 10, "mmamammmmammamamaaamammma" );
	trap_ConfigString( CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba" );

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	trap_ConfigString( CS_LIGHTS + 63, "a" );
}

const char *ED_ParseEntity( const char *data, gentity_t *ent );

/*
* G_InitLevel
* reinitializes the level subsystems and the map entities. Called
* from the server on map load, or by the game code on match restarts
*/
void G_InitLevel( const char *mapname, const char *entities, int entstrlen, unsigned int serverTime )
{
	char hostname[MAX_CONFIGSTRING_CHARS];
	char name[MAX_CONFIGSTRING_CHARS];
	char *mapString = NULL;
	gentity_t *ent = NULL;
	char *token;

	game.serverTime = serverTime;

	GS_FreeSpacePartition();
	G_asShutdownGametypeScript();

	if( !entities )
		GS_Error( "SpawnEntities: No entities list" );

	// make a copy of the raw entities string so it's not freed with the pool
	mapString = ( char * )G_Malloc( entstrlen + 1 );
	memcpy( mapString, entities, entstrlen );
	Q_strncpyz( name, mapname, sizeof( name ) );

	// clear old data
	G_LevelInitPool( strlen(mapname) + 1 + (entstrlen + 1) * 2 + G_LEVEL_DEFAULT_POOL_SIZE );
	memset( &level, 0, sizeof( level ) );
	memset( &gs.gameState, 0, sizeof( gs.gameState ) );

	// get the string back
	Q_strncpyz( level.mapname, name, sizeof( level.mapname ) );
	level.mapString = ( char * )G_LevelMalloc( entstrlen + 1 );
	level.mapStrlen = entstrlen;
	memcpy( level.mapString, mapString, entstrlen );
	G_Free( mapString );
	mapString = NULL;

	//
	// initialize
	//

	level.mapTimeStamp = game.serverTime;

	GS_CreateSpacePartition();

	Q_strncpyz( hostname, trap_Cvar_String( "sv_hostname" ), sizeof( hostname ) );
	trap_ConfigString( CS_HOSTNAME, hostname );

	trap_ConfigString( CS_MAPNAME, level.mapname );
	trap_ConfigString( CS_MESSAGE, level.mapname );
	trap_ConfigString( CS_SKYBOX, "" );
	trap_ConfigString( CS_AUDIOTRACK, "" );

	// initialize level subsystems
	G_InitGameCommands();
	G_PrecacheLightStyles();
	G_PrecacheItems();
	G_PrecacheLocalSounds();
	G_Damage_Init();
	G_Teams_Init();
	G_SpawnQueue_Init();
	G_Gametype_Init();
	G_Buildables_Init();
	G_PlayerClass_RegisterClasses();

	// reset game.numentities so G_Spawn won't allocate new edicts
	game.numentities = gs.maxclients;
	level.canSpawnEntities = qtrue;

	// spawn map entities
	entities = level.mapString;
	while( entities )
	{
		token = COM_Parse( &entities );
		if( !token[0] )
			break;

		if( token[0] != '{' )
			GS_Error( "G_SpawnMapEntities: found %s when expecting {", token );

		if( !ent ) // first entity is always the worldspawn
		{
			ent = worldEntity;
			G_InitEntity( ent );
		}
		else
			ent = G_Spawn();

		// parse the entity string
		entities = ED_ParseEntity( entities, ent );

		if( !G_CallSpawn( ent ) ) // call this entity spawning function
		{
			G_FreeEntity( ent );
			ent->freetimestamp = 0; // don't wait for reusing it
			continue;
		}
	}
}
