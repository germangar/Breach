/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

/*
* G_info_player_start
*/
void G_info_player_start( gentity_t *ent )
{
	static const char *classname = "info_player_start";
	G_DropSpawnpointToFloor( ent );
	ent->classname = classname;
}

/*
* G_Spawnpoint_Spawn
*/
void G_Spawnpoint_Spawn( gentity_t *ent )
{
	G_DropSpawnpointToFloor( ent );
}

/*
* G_OffsetSpawnPoint - Spawn telefragging protection
*/
qboolean G_OffsetSpawnPoint( vec3_t origin, vec3_t box_mins, vec3_t box_maxs, float radius, qboolean checkground )
{
	trace_t	trace;
	vec3_t virtualorigin;
	vec3_t absmins, absmaxs;
	float playerbox_rowwidth;
	float playerbox_columnwidth;
	int rows, columns;
	int i, j, row = 0, column = 0;
	int rowseed = rand() & 255;
	int columnseed = rand() & 255;
	int mask_spawn = MASK_PLAYERSOLID | ( CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_TELEPORTER|CONTENTS_JUMPPAD|CONTENTS_BODY|CONTENTS_NODROP );
	int playersFound = 0, worldfound = 0, nofloorfound = 0, badclusterfound = 0;
	// check box clusters
	int cluster;
	int num_leafs;
	int leafs[8];

	if( radius <= box_maxs[0] - box_mins[0] )
		return qtrue;

	playerbox_rowwidth = 2 + box_maxs[0] - box_mins[0];
	playerbox_columnwidth = 2 + box_maxs[1] - box_mins[1];

	rows = radius / playerbox_rowwidth;
	columns = radius / playerbox_columnwidth;

	// no, we won't just do a while, let's go safe and just check as many times as
	// positions in the grid. If we didn't found a spawnpoint by then, we let it telefrag.
	for( i = 0; i < ( rows * columns ); i++ )
	{
		row = Q_brandom( &rowseed, -rows, rows );
		column = Q_brandom( &columnseed, -columns, columns );

		VectorSet( virtualorigin, origin[0] + ( row * playerbox_rowwidth ),
			origin[1] + ( column * playerbox_columnwidth ),
			origin[2] );

		VectorAdd( virtualorigin, box_mins, absmins );
		VectorAdd( virtualorigin, box_maxs, absmaxs );
		for( j = 0; j < 2; j++ )
		{
			absmaxs[j] += 1;
			absmins[j] -= 1;
		}

		//check if position is inside world

		// check if valid cluster
		cluster = -1;
		num_leafs = trap_CM_BoxLeafnums( absmins, absmaxs, leafs, 8, NULL );
		for( j = 0; j < num_leafs; j++ )
		{
			cluster = trap_CM_LeafCluster( leafs[j] );
			if( cluster == -1 )
				break;
		}

		if( cluster == -1 )
		{
			badclusterfound++;
			continue;
		}

		// one more trace is needed, only checking if some part of the world is on the
		// way from spawnpoint to the virtual position
		trap_CM_TransformedBoxTrace( &trace, origin, virtualorigin, box_mins, box_maxs, NULL, MASK_PLAYERSOLID, NULL, NULL );
		if( trace.fraction != 1.0f )
			continue;

		// check if anything solid is on player's way

		GS_Trace( &trace, vec3_origin, absmins, absmaxs, vec3_origin, ENTITY_WORLD, mask_spawn, 0 );
		if( trace.startsolid || trace.allsolid || trace.ent != -1 )
		{
			if( trace.ent == 0 )
				worldfound++;
			else if( trace.ent < gs.maxclients )
				playersFound++;
			continue;
		}

		// one more check before accepting this spawn: there's ground at our feet?
		if( checkground ) // if floating item flag is not set
		{
			vec3_t origin_from, origin_to;

			VectorCopy( virtualorigin, origin_from );
			origin_from[2] += box_mins[2] + 1;
			VectorCopy( origin_from, origin_to );
			origin_to[2] -= 32;

			// use point trace instead of box trace to avoid small glitches that can't support the player but will stop the trace
			GS_Trace( &trace, origin_from, vec3_origin, vec3_origin, origin_to, -1, MASK_PLAYERSOLID, 0 );
			if( trace.startsolid || trace.allsolid || trace.fraction == 1.0f )
			{
				// full run means no ground
				nofloorfound++;
				continue;
			}
		}

		VectorCopy( virtualorigin, origin );
		return qtrue;
	}

	return qfalse;
}

/*
* G_SelectSpawnPoint
*/
static void G_SelectSpawnPoint( int team, vec3_t box_mins, vec3_t box_maxs, vec3_t origin, vec3_t angles )
{
	gentity_t *spot = NULL;

	while( ( spot = G_Find( spot, FOFFSET( gentity_t, classname ), "info_spawnpoint" ) ) != NULL )
	{
		if( team == spot->s.team )
			break;
	}

	if( !spot )
	{
		// no spawnpoint present, allow quake's classic "info_player_start" for any team
		while( ( spot = G_Find( spot, FOFFSET( gentity_t, classname ), "info_player_start" ) ) != NULL )
		{
			if( spot )
				break;
		}
	}

	if( !spot ) // if no spawnpoint found, use world origin
		spot = worldEntity;

	VectorCopy( spot->s.ms.origin, origin );
	VectorCopy( spot->s.ms.angles, angles );

	G_OffsetSpawnPoint( origin, box_mins, box_maxs, 250, ( qboolean )!( spot->spawnflags & 1 ) );
}

/*
* G_SelectClientSpawnPoint
*/
qboolean G_SelectClientSpawnPoint( gentity_t *ent, int team, vec3_t spawn_origin, vec3_t spawn_angles )
{
	gsplayerclass_t *playerClass;
	
	if( !ent->client )
		return qfalse;

	playerClass = GS_PlayerClassByIndex( ent->client->playerClassIndex );
	if( !playerClass )
		return qfalse;

	G_SelectSpawnPoint( team, playerClass->mins, playerClass->maxs, spawn_origin, spawn_angles );

	return qtrue;
}

//==================================================
// CLIENTS SPAWN QUEUE
//==================================================

cvar_t *g_spawnsystem;
cvar_t *g_spawnsystem_wave_time;
cvar_t *g_spawnsystem_wave_maxcount;

#define REINFORCEMENT_WAVE_DELAY		5  // seconds
#define REINFORCEMENT_WAVE_MAXCOUNT	    16

typedef struct
{
	int list[MAX_CLIENTS];
	int head;
	int start;
	int system;
	int wave_time;
	int wave_maxcount;
	qboolean spectate_team;
	unsigned int nextWaveTime;
} g_teamspawnqueue_t;

g_teamspawnqueue_t g_spawnQueues[GS_NUMTEAMS];

/*
* G_SpawnQueue_SetTeamSpawnsystem
*/
void G_SpawnQueue_SetTeamSpawnsystem( int team, int spawnsystem, int wave_time, int wave_maxcount, qboolean spectate_team )
{
	g_teamspawnqueue_t *queue;

	if( team < TEAM_NOTEAM || team >= GS_NUMTEAMS )
		return;

	queue = &g_spawnQueues[team];
	if( queue->system == spawnsystem )
		return;

	if( wave_time && wave_time != queue->wave_time )
		queue->nextWaveTime = level.time + brandom( 0, wave_time * 1000 );

	queue->system = spawnsystem;
	queue->wave_time = wave_time;
	queue->wave_maxcount = wave_maxcount;
	if( spawnsystem != SPAWNSYSTEM_INSTANT )
		queue->spectate_team = spectate_team;
}

/*
* G_SpawnQueue_Init
*/
void G_SpawnQueue_Init( void )
{
	int spawnsystem, team;

	g_spawnsystem = trap_Cvar_Get( "g_spawnsystem", va( "%i", SPAWNSYSTEM_WAVES ), CVAR_DEVELOPER );
	g_spawnsystem_wave_time = trap_Cvar_Get( "g_spawnsystem_wave_time", va( "%i", REINFORCEMENT_WAVE_DELAY ), CVAR_ARCHIVE );
	g_spawnsystem_wave_maxcount = trap_Cvar_Get( "g_spawnsystem_wave_maxcount", va( "%i", REINFORCEMENT_WAVE_MAXCOUNT ), CVAR_ARCHIVE );

	memset( g_spawnQueues, 0, sizeof( g_spawnQueues ) );
	for( team = TEAM_NOTEAM; team < GS_NUMTEAMS; team++ )
		memset( &g_spawnQueues[team].list, -1, sizeof( g_spawnQueues[0].list ) );

	spawnsystem = g_spawnsystem->integer;
	clamp( spawnsystem, SPAWNSYSTEM_INSTANT, SPAWNSYSTEM_HOLD );
	if( spawnsystem != g_spawnsystem->integer )
		trap_Cvar_ForceSet( "g_spawnsystem", va( "%i", spawnsystem ) );

	for( team = TEAM_NOTEAM; team < GS_NUMTEAMS; team++ )
	{
		if( team == TEAM_SPECTATOR )
			G_SpawnQueue_SetTeamSpawnsystem( team, SPAWNSYSTEM_INSTANT, 0, 0, qfalse );
		else
			G_SpawnQueue_SetTeamSpawnsystem( team, spawnsystem, g_spawnsystem_wave_time->integer, g_spawnsystem_wave_maxcount->integer, qtrue );
	}
}

/*
* G_RespawnTime
*/
int G_SpawnQueue_NextRespawnTime( int team )
{
	int time;

	if( g_spawnQueues[team].system != SPAWNSYSTEM_WAVES )
		return 0;

	if( g_spawnQueues[team].nextWaveTime < level.time )
		return 0;

	time = (int)( g_spawnQueues[team].nextWaveTime - level.time );
	return ( time > 0 ) ? time : 0;
}

/*
* G_SpawQueue_RemoveClient - Check all queues for this client and remove it
*/
void G_SpawQueue_RemoveClient( gentity_t *ent )
{
	g_teamspawnqueue_t *queue;
	int i, team;

	if( !ent->client )
		return;

	for( team = TEAM_NOTEAM; team < GS_NUMTEAMS; team++ )
	{
		queue = &g_spawnQueues[team];
		for( i = queue->start; i < queue->head; i++ )
		{
			if( queue->list[i % MAX_CLIENTS] == ENTNUM( ent ) )
				queue->list[i % MAX_CLIENTS] = -1;
		}
	}
}

/*
* G_SpawnQueue_ResetTeamQueue
*/
void G_SpawnQueue_ResetTeamQueue( int team )
{
	g_teamspawnqueue_t *queue;

	if( team < TEAM_NOTEAM || team >= GS_NUMTEAMS )
		return;

	queue = &g_spawnQueues[team];
	memset( &queue->list, -1, sizeof( queue->list ) );
	queue->head = queue->start = 0;
	queue->nextWaveTime = 0;
}

/*
* G_SpawnQueue_ReleaseTeamQueue
*/
void G_SpawnQueue_ReleaseTeamQueue( int team )
{
	g_teamspawnqueue_t *queue;
	gentity_t *ent;
	int count;
	qboolean ghost;

	if( team < TEAM_NOTEAM || team >= GS_NUMTEAMS )
		return;

	queue = &g_spawnQueues[team];

	if( queue->start >= queue->head )
		return;

	// try to spawn them
	for( count = 0; ( queue->start < queue->head ) && ( count < gs.maxclients ); queue->start++, count++ )
	{
		if( queue->list[queue->start % MAX_CLIENTS] < 0 || queue->list[queue->start % MAX_CLIENTS] >= gs.maxclients )
			continue;

		ent = &game.entities[ queue->list[queue->start % MAX_CLIENTS] ];

		ghost = qfalse;
		if( team == TEAM_SPECTATOR )
			ghost = qtrue;

		// if unable to spawn the next client in the queue, stop,
		// so next time the queue starts from this client again
		if( !G_Client_Respawn( ent, ghost ) )
			break;

		// when spawning inside spectator team bring up the chase camera
		//if( team == TEAM_SPECTATOR && !ent->client->resp.chase.active )
		//	G_ChasePlayer( ent, NULL, qfalse, qfalse );
	}
}

/*
* G_SpawnQueue_AddClient
*/
void G_SpawnQueue_AddClient( gentity_t *ent )
{
	g_teamspawnqueue_t *queue;
	int i;

	if( !ent->client )
		return;

	if( ENTNUM( ent ) < 0 || ENTNUM( ent ) >= gs.maxclients )
		return;

	if( ent->client->team < TEAM_NOTEAM || ent->client->team >= GS_NUMTEAMS )
		return;

	queue = &g_spawnQueues[ent->client->team];

	for( i = queue->start; i < queue->head; i++ )
	{
		if( queue->list[i % MAX_CLIENTS] == ENTNUM( ent ) )
			return;
	}

	G_SpawQueue_RemoveClient( ent );
	queue->list[queue->head % MAX_CLIENTS] = ENTNUM( ent );
	queue->head++;

	//if( queue->spectate_team )
	//	G_ChasePlayer( ent, NULL, qtrue, 0 );
}

/*
* G_SpawnQueue_Think
*/
void G_SpawnQueue_Think( void )
{
	int team, maxCount, count, spawnSystem;
	g_teamspawnqueue_t *queue;
	gentity_t *ent;
	qboolean ghost;

	for( team = TEAM_NOTEAM; team < GS_NUMTEAMS; team++ )
	{
		queue = &g_spawnQueues[team];

		// if the system is limited, set limits
		maxCount = MAX_CLIENTS;

		spawnSystem = queue->system;
		clamp( spawnSystem, SPAWNSYSTEM_INSTANT, SPAWNSYSTEM_HOLD );

		switch( spawnSystem )
		{
		case SPAWNSYSTEM_INSTANT:
		default:
			break;
		case SPAWNSYSTEM_WAVES:
			if( queue->nextWaveTime > level.time )
			{
				maxCount = 0;
			}
			else
			{
				maxCount = ( g_spawnsystem_wave_maxcount->integer < 1 ) ? 1 : g_spawnsystem_wave_maxcount->integer; // max count per reinforcement wave
				queue->nextWaveTime = level.time + ( queue->wave_time * 1000 );
			}
			break;
		case SPAWNSYSTEM_HOLD:
			maxCount = 0; // players wait to be spawned elsewhere
			break;
		}

		if( maxCount <= 0 )
			continue;

		if( queue->start >= queue->head )
			continue;

		// try to spawn them
		for( count = 0; ( queue->start < queue->head ) && ( count < maxCount ); queue->start++, count++ )
		{
			if( queue->list[queue->start % MAX_CLIENTS] < 0 || queue->list[queue->start % MAX_CLIENTS] >= gs.maxclients )
				continue;

			ent = &game.entities[queue->list[queue->start % MAX_CLIENTS]];

			ghost = qfalse;
			if( team == TEAM_SPECTATOR )
				ghost = qtrue;

			// if unable to spawn the next client in the queue, stop,
			// so next time the queue starts from this client again
			if( !G_Client_Respawn( ent, ghost ) )
				break;

			// when spawning inside spectator team bring up the chase camera
			//if( team == TEAM_SPECTATOR && !ent->client->resp.chase.active )
			//	G_ChasePlayer( ent, NULL, qfalse, qfalse );
		}
	}
}
