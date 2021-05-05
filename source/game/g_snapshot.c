/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

//==================================================
// SNAPSHOT
//==================================================

static int entityAddedToSnapList[MAX_EDICTS];
static int entitiesDroppedFromSnapList;

/*
* GainForAttenuation
*/
static float GainForAttenuation( float dist, float attenuation )
{
	float gain;
	cvar_t *s_attenuation_model = trap_Cvar_Get( "s_attenuation_model", S_DEFAULT_ATTENUATION_MODEL, CVAR_DEVELOPER );
	cvar_t *s_attenuation_maxdistance = trap_Cvar_Get( "s_attenuation_maxdistance", S_DEFAULT_ATTENUATION_MAXDISTANCE, CVAR_DEVELOPER );
	cvar_t *s_attenuation_refdistance = trap_Cvar_Get( "s_attenuation_refdistance", S_DEFAULT_ATTENUATION_REFDISTANCE, CVAR_DEVELOPER );

	switch( s_attenuation_model->integer )
	{
	case 0:
		//gain = (1 – AL_ROLLOFF_FACTOR * (distance – AL_REFERENCE_DISTANCE) / (AL_MAX_DISTANCE – AL_REFERENCE_DISTANCE))
		//AL_LINEAR_DISTANCE
		dist = min( dist, s_attenuation_maxdistance->value );
		gain = ( 1 - attenuation * ( dist - s_attenuation_refdistance->value ) / ( s_attenuation_maxdistance->value - s_attenuation_refdistance->value ) );
		break;
	case 1:
	default:
		//gain = (1 – AL_ROLLOFF_FACTOR * (distance – AL_REFERENCE_DISTANCE) / (AL_MAX_DISTANCE – AL_REFERENCE_DISTANCE))
		//AL_LINEAR_DISTANCE_CLAMPED
		dist = max( dist, s_attenuation_refdistance->value );
		dist = min( dist, s_attenuation_maxdistance->value );
		gain = ( 1 - attenuation * ( dist - s_attenuation_refdistance->value ) / ( s_attenuation_maxdistance->value - s_attenuation_refdistance->value ) );
		break;
	case 2:
		//gain = AL_REFERENCE_DISTANCE / (AL_REFERENCE_DISTANCE + AL_ROLLOFF_FACTOR * (distance – AL_REFERENCE_DISTANCE));
		//AL_INVERSE_DISTANCE
		gain = s_attenuation_refdistance->value / ( s_attenuation_refdistance->value + attenuation * ( dist - s_attenuation_refdistance->value ) );
		break;
	case 3:
		//AL_INVERSE_DISTANCE_CLAMPED
		//gain = AL_REFERENCE_DISTANCE / (AL_REFERENCE_DISTANCE + AL_ROLLOFF_FACTOR * (distance – AL_REFERENCE_DISTANCE));
		dist = max( dist, s_attenuation_refdistance->value );
		dist = min( dist, s_attenuation_maxdistance->value );
		gain = s_attenuation_refdistance->value / ( s_attenuation_refdistance->value + attenuation * ( dist - s_attenuation_refdistance->value ) );
		break;
	case 4:
		//AL_EXPONENT_DISTANCE
		//gain = (distance / AL_REFERENCE_DISTANCE) ^ (- AL_ROLLOFF_FACTOR)
		gain = pow( ( dist / s_attenuation_refdistance->value ), ( -attenuation ) );
		break;
	case 5:
		//AL_EXPONENT_DISTANCE_CLAMPED
		//gain = (distance / AL_REFERENCE_DISTANCE) ^ (- AL_ROLLOFF_FACTOR)
		dist = max( dist, s_attenuation_refdistance->value );
		dist = min( dist, s_attenuation_maxdistance->value );
		gain = pow( ( dist / s_attenuation_refdistance->value ), ( -attenuation ) );
		break;
	case 6:
		// qfusion gain
		dist -= 80;
		if( dist < 0 ) dist = 0;
		gain = 1.0 - dist * attenuation * 0.0001;
		break;
	}

	return gain;
}

/*
* G_AddEntNumToSnapList
*/
static void G_AddEntNumToSnapList( int entNum, snapshotEntityNumbers_t *entsList )
{
	// ignore if exceeds max count
	if( entsList->numSnapshotEntities >= MAX_SNAPSHOT_ENTITIES )
	{
		entitiesDroppedFromSnapList++;
		GS_Printf( "G_AddEntNumToSnapList: MAX_SNAPSHOT_ENTITIES reached. Dropping %i entities\n", entitiesDroppedFromSnapList );
		return;
	}

	// don't double add entities
	if( entityAddedToSnapList[entNum] )
		return;

	entsList->snapshotEntities[entsList->numSnapshotEntities] = entNum;
	entsList->numSnapshotEntities++;
	entityAddedToSnapList[entNum] = qtrue;
}

/*
* G_SnapCullEntity
*/
static qboolean G_SnapCullEntity( gentity_t *ent, gentity_t *viewer, vec3_t vieworg )
{
	// filters: transmit only to clients in the same team as this entity
	if( ( ent->netflags & SVF_ONLYTEAM ) && ( viewer && ent->s.team != viewer->s.team ) )
		return qtrue;

	// filters: send to everyone
	if( ent->netflags & SVF_BROADCAST )
		return qfalse;

	// filters: sound entities are culled by distance
	if( ent->s.type == ET_SOUNDEVENT )
	{
		float dist, gain;

		if( !ent->s.skinindex )
			return qfalse;

		if( ent->s.modelindex2 )
		{
			gentity_t *owner = &game.entities[ent->s.modelindex2];
			// event entities can't be the owner of sound entities
			if( ISEVENTENTITY( &owner->s ) )
				return qtrue;
		}

		if( trap_PHSCullEntity( ent->vis.headnode, ent->vis.num_clusters, ent->vis.clusternums ) )
			return qtrue;

		dist = DistanceFast( ent->s.ms.origin, vieworg ) - 256; // extend the influence sphere cause the player could be moving
		gain = GainForAttenuation( dist < 0 ? 0 : dist, (float)( ent->s.skinindex / 16.0f ) );
		if( gain > 0.03 )  // curved attenuations can keep barely audible sounds for long distances
			return qfalse;

		return qtrue;
	}

	// if the entity is only a sound
	if( !ent->s.modelindex1 && !ent->s.events[0] && !ent->s.effects && ent->s.sound )
	{
		float dist, gain;

		if( trap_PHSCullEntity( ent->vis.headnode, ent->vis.num_clusters, ent->vis.clusternums ) )
			return qtrue;

		dist = DistanceFast( ent->s.ms.origin, vieworg ) - 256; // extend the influence sphere cause the player could be moving
		gain = GainForAttenuation( dist < 0 ? 0 : dist, ATTN_STATIC );
		if( gain > 0.03 )                              
			return qfalse;

		return qtrue;
	}

	// check for being outside of the visibility distance
	if( level.farplanedist )
	{
		vec3_t absmins, absmaxs;
		float radius, movedist;

		// extend the radius accounting the viewer movement
		movedist = VectorLengthFast( viewer->s.ms.velocity ) * ( ( abs( viewer->client->timeDelta ) + gs.snapFrameTime ) * 0.001f );
		radius = max( level.farplanedist, 512 ) + movedist;
		VectorAdd( ent->s.ms.origin, ent->s.local.mins, absmins );
		VectorAdd( ent->s.ms.origin, ent->s.local.maxs, absmaxs );

		if( !BoundsAndSphereIntersect( absmins, absmaxs, vieworg, radius ) )
			return qtrue;
	}

	if( trap_AreaCullEntity( ent->vis.areanum, ent->vis.areanum2 ) )
		return qtrue;

	if( trap_PVSCullEntity( ent->vis.headnode, ent->vis.num_clusters, ent->vis.clusternums ) )
		return qtrue;

	return qfalse; // not culled
}

/*
* G_BuildSnapEntitiesList
*/
void G_BuildSnapEntitiesList( int clientNum, snapshotEntityNumbers_t *entsList, qboolean cull )
{
	int entNum;
	gentity_t *ent;
	gentity_t *viewer;
	vec3_t vieworg;

	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "G_BuildSnapEntitiesList: Bad client number\n" );

	entitiesDroppedFromSnapList = 0;
	memset( entityAddedToSnapList, 0, sizeof( int ) * MAX_EDICTS );

	viewer = &game.entities[clientNum];

	VectorSet( vieworg, viewer->s.ms.origin[0], viewer->s.ms.origin[1],
		viewer->s.ms.origin[2] + game.clients[clientNum].ps.viewHeight );

	trap_SetVIS( vieworg, qfalse );

	// always add the client entity to snap list, no matter what
	G_AddEntNumToSnapList( viewer->s.number, entsList );

	// if out of the world don't add any entities but the client
	if( GClip_PointCluster( vieworg ) == -1 )
		return;

	// make a pass checking for portals and merge PVS in case of finding any
	for( entNum = 0; entNum < game.numentities; entNum++ )
	{
		ent = game.entities + entNum;

		// skyportals are always svf_noclient, but they merge pvs
		if( ent->s.type == ET_SKYPORTAL )
		{
			if( !( ent->s.effects & 1 ) )
				trap_SetVIS( ent->s.ms.origin, qtrue );
			continue;
		}

		if( ent->netflags & SVF_NOCLIENT )
			continue;

		if( ent->s.type == ET_PORTALSURFACE )
		{                               // merge PV sets if portal
			if( cull && G_SnapCullEntity( ent, viewer, vieworg ) )
				continue;

			// if origin differs it's a portal, otherwise it's a mirror.
			if( !VectorCompare( ent->s.ms.origin, ent->s.origin2 ) )
			{
				trap_SetVIS( ent->s.origin2, qtrue );
			}
		}
	}

	for( entNum = 0; entNum < game.numentities; entNum++ )
	{
		ent = game.entities + entNum;

		if( ent->netflags & SVF_NOCLIENT )
			continue;

		if( cull && G_SnapCullEntity( ent, viewer, vieworg ) )
			continue;

		G_AddEntNumToSnapList( entNum, entsList );
	}
}

/*
* G_GetEntityState - Used by the server to access entity states
*/
entity_state_t *G_GetEntityState( int entNum )
{
	assert( entNum >= 0 && entNum < MAX_EDICTS );
	return &game.entities[entNum].s;
}

/*
* G_GetPlayerState - Used by the server to access player states
*/
player_state_t *G_GetPlayerState( int clientNum )
{
	assert( clientNum >= 0 && clientNum < gs.maxclients );
	return &game.clients[clientNum].ps;
}

/*
* G_GetGameState - Used by the server to access game state stats
*/
game_state_t *G_GetGameState( void )
{
	return &gs.gameState;
}
