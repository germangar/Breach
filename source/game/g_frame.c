/*
*/

#include "g_local.h"

/*
* G_CheckCvars
* Check for cvars that have been modified and need the game to be updated
*/
static void G_CheckCvarUpdates( void )
{
}

//===================================================================
//		SNAP FRAMES
//===================================================================

static qboolean g_snapStarted = qfalse;

/*
* G_StartFrameSnap
* a snap was just sent, set up for new one
*/
static void G_StartFrameSnap( void )
{
	g_snapStarted = qtrue;
}

/*
* G_ClearSnap
* We just run G_SnapFrame, the server just sent the snap to the clients,
* it's now time to clean up snap specific data to start the next snap from clean.
*/
void G_ClearSnap( void )
{
	gentity_t *ent;

	// clear all events to free entity spots
	for( ent = game.entities; ENTNUM( ent ) < game.numentities; ent++ )
	{
		// events only last for a single message
		ent->s.events[0] = ent->s.events[1] = 0;
		ent->s.eventParms[0] = ent->s.eventParms[1] = qfalse;
		ent->eventPriority[0] = ent->eventPriority[1] = qfalse;
		ent->s.flags &= ~( SFL_TELEPORTED ); // force these bits to be set each snap.

		if( !ent->s.local.inuse )
			continue;

		// clear the snap temp info
		memset( &ent->snap, 0, sizeof( ent->snap ) );
		if( ent->client )
			memset( &ent->client->spawn.snap, 0, sizeof( client_snap_t ) );

		if( ent->clearSnap )
		{
			ent->clearSnap( ent );
			if( !ent->s.local.inuse )  // entity might be freed here
				continue;
		}
	}

	g_snapStarted = qfalse;
}

/*
* G_SnapFrame
* It's time to send a new snap, so set the world up for sending
*/
int G_SnapFrame( unsigned int snapNum )
{
	gentity_t *ent;

	// finish entities snapshot for transmission
	for( ent = game.entities; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( ent->s.local.inuse )
		{
			if( ent->closeSnap )
				ent->closeSnap( ent );
			if( !ent->s.local.inuse )
				continue; // was freed?
			if( ent->client )
				G_ClientCloseSnap( ent );
		}
	}

	// check up entities for transmission
	for( ent = game.entities; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( ent->s.number != ENTNUM( ent ) )
		{
			if( developer->integer )
				GS_Printf( "fixing ent->s.number (etype:%i, classname:%s)\n", ent->s.type, ent->classname ? ent->classname : "noclassname" );
			ent->s.number = ENTNUM( ent );
		}

		GClip_SetEntityAreaInfo( ent ); // updates r.areanums and door areaportal states
	}

	return game.numentities;
}

//===================================================================
//		WORLD FRAMES
//===================================================================

/*
* G_RunEntity
*/
static void G_RunEntity( gentity_t *ent )
{
	if( ISEVENTENTITY( &ent->s ) )  // events do not think
		return;

	// perform some sanity checks
	if( ent->timeDelta && ( ent->s.solid != SOLID_PROJECTILE ) )
	{
		GS_Printf( "Warning: G_RunEntity: Fixing timeDelta on non projectile entity\n" );
		ent->timeDelta = 0;
	}

	if( ent->s.ms.linearProjectileTimeStamp )
	{
		if( ent->s.ms.type != MOVE_TYPE_LINEAR_PROJECTILE )
		{
			GS_Printf( "Warning: G_RunEntity: Fixing  MOVE_TYPE_LINEAR_PROJECTILE not assigned to entity with linearProjectileTimeStamp\n" );
			ent->s.ms.type = MOVE_TYPE_LINEAR_PROJECTILE;
		}
	}

	// run the thinking event
	if( ent->nextthink > 0 && ent->nextthink <= level.time && !GS_Paused() )
	{
		ent->nextthink = 0;
		if( ent->think )
			ent->think( ent );
	}

	// run client thinking
	if( ent->client )
	{
		if( ent->client->fakeClient )
		{
			ent->client->spawn.fakeClientUCmd.msec = game.framemsecs;
			G_ClientThink( ENTNUM( ent ), &ent->client->spawn.fakeClientUCmd, 0 );
			memset( &ent->client->spawn.fakeClientUCmd, 0, sizeof( usercmd_t ) );
		}
		else
		{
			trap_ExecuteClientThinks( ENTNUM( ent ) );
		}

		return; // client physics are run in the clientThink function
	}

	// run physics frame
	G_RunPhysicsTick( ent, NULL, game.framemsecs );
}

/*
* G_RunEntities
* treat each object in turn
* even the world and clients get a chance to think
*/
static void G_RunEntities( void )
{
	gentity_t *ent;
	int i, step;

	// run clients first, once in ascending and the next in descending order
	if( level.framenum & 1 )
	{
		i = gs.maxclients - 1;
		step = -1;
	}
	else
	{
		i = 0;
		step = 1;
	}

	for( ; i < gs.maxclients && i >= 0; i += step )
	{
		ent = game.entities + i;
		G_RunEntity( ent );
	}

	// if paused don't bother running entities, it will do nothing
	if( GS_Paused() )
		return;

	// run non-pushers second
	for( ent = game.entities + gs.maxclients; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( ent->s.local.inuse && ent->s.ms.type != MOVE_TYPE_PUSHER )
			G_RunEntity( ent );
	}

	// run pushers last
	for( ent = game.entities + gs.maxclients; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( ent->s.local.inuse && ent->s.ms.type == MOVE_TYPE_PUSHER )
			G_RunEntity( ent );
	}
}

/*
* G_RunFrame
* Advances the world
*/
void G_RunFrame( unsigned int msec, unsigned int serverTime )
{
	G_CheckCvarUpdates();

	game.framemsecs = msec;
	game.serverTime = serverTime;

	if( !g_snapStarted )
		G_StartFrameSnap();

	// update time and pause
	if( GS_Paused() )
	{
		gs.gameState.longstats[GAMELONG_MATCHSTART] += game.framemsecs;
		level.pausedTime += msec;
		game.framemsecs = 0;
	}
	else
	{
		level.time = game.serverTime - ( level.mapTimeStamp + level.pausedTime );
		level.framenum++;
	}

	G_Gametype_AdvanceMatchState();
	G_SpawnQueue_Think();

	G_RunEntities();
	G_asCallRunFrameScript();

	if( !GS_Paused() )
		GClip_BackUpCollisionFrame();
}
