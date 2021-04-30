/*
   Copyright (C) 2007 German Garcia
 */

#include "g_local.h"

/*
* G_InitEntity
*/
void G_InitEntity( gentity_t *ent )
{
	static const char noclassname[] = "";

	assert( level.canSpawnEntities );

	memset( ent, 0, sizeof( *ent ) );
	ent->s.number = ENTNUM( ent );
	ent->s.type = ET_NODRAW;
	ent->s.local.inuse = qtrue;
	ent->netflags = SVF_NOCLIENT;
	ent->classname = noclassname;
	ent->timestamp = level.time;
	ent->env.groundentity = ENTITY_INVALID;
	ent->asSpawnFuncID = ent->asThinkFuncID = ent->asTouchFuncID = ent->asActivateFuncID = ent->asPainFuncID = ent->asBlockedFuncID = ent->asDieFuncID = ent->asStopFuncID = -1;

	if( ent->s.number >= 0 && ent->s.number < gs.maxclients )
		ent->client = game.clients + ent->s.number;
}

/*
* G_FreeEntity
*/
void G_FreeEntity( gentity_t *ent )
{
	qboolean instantRelease = !ent->transmitted;

	if( ISEVENTENTITY( &ent->s ) ) // event entities don't need to wait
		instantRelease = qtrue;

	if( level.mapTimeStamp == game.serverTime ) // everything can be reused during initialization
		instantRelease = qtrue;

	GClip_UnlinkEntity( ent );

	G_InitEntity( ent );
	ent->s.local.inuse = qfalse;

	if( !instantRelease )
		ent->freetimestamp = trap_Milliseconds();
}

/*
* G_Door_SetAreaportalState
*/
void G_Door_SetAreaportalState( gentity_t *ent, qboolean open )
{
	//GS_Printf( "Areaportal set to %s\n", open ? "OPEN" : "CLOSED" );
	ent->door_portal_state = open;
}

/*
* G_RotationFromAngles
* finds a rotation direction clock wise along the axis matching the angles looking direction.
* angles looking direction is an indication, and forced to fix identity axises.
*/
void G_RotationFromAngles( vec3_t angles, vec3_t movedir )
{
	vec3_t dir;

	// if no angle is set, use Y axis clock counter
	if( !angles[0] && !angles[1] && !angles[2] )
		VectorSet( angles, -90, 90, 0 );

	AngleVectors( angles, dir, NULL, NULL );
	if( DotProduct( dir, axis_identity[FORWARD] ) > 0.1 )
	{
		VectorSet( movedir, 0, 0, -1 );
	}
	else if( -DotProduct( dir, axis_identity[FORWARD] ) > 0.1 )
	{
		VectorSet( movedir, 0, 0, 1 );
	}
	if( DotProduct( dir, axis_identity[RIGHT] ) > 0.1 )
	{
		VectorSet( movedir, -1, 0, 0 );
	}
	else if( -DotProduct( dir, axis_identity[RIGHT] ) > 0.1 )
	{
		VectorSet( movedir, 1, 0, 0 );
	}
	if( DotProduct( dir, axis_identity[UP] ) > 0.1 )
	{
		VectorSet( movedir, 0, -1, 0 );
	}
	else if( -DotProduct( dir, axis_identity[UP] ) > 0.1 )
	{
		VectorSet( movedir, 0, 1, 0 );
	}
}

/*
* G_FireTouches
*/
void G_FireTouches( gentity_t *ent, touchlist_t *touchList )
{
	int i;
	gentity_t *touched;
	cplane_t *plane;
	int surfaceFlags;

	GClip_TouchTriggers( ent );

	if( !touchList || !touchList->numtouch )
		return;

	// touch other objects
	if( ent->s.solid != SOLID_NOT )
	{
		for( i = 0; i < touchList->numtouch; i++ )
		{
			touched = &game.entities[touchList->touchents[i]];
			plane = &touchList->touchplanes[i];
			surfaceFlags = touchList->touchsurfs[i];

			if( ent->s.local.inuse && touched->s.local.inuse && touched->touch )
				touched->touch( touched, ent, NULL, 0 );

			if( ent->s.local.inuse && touched->s.local.inuse && ent->touch )
				ent->touch( ent, touched, plane, surfaceFlags );
		}
	}
}

//==================================================
// TARGETS
//==================================================

/*
* G_IsTargetOfEnt
*/
qboolean G_IsTargetOfEnt( gentity_t *ent, gentity_t *check )
{
	if( ent && ent->target && check && check->targetname )
	{
		return ( qboolean )( Q_stricmp( ent->target, check->targetname ) == 0 );
	}

	return qfalse;
}

/*
* G_GenerateLocalTargetName - Used for spawning auto-triggers
*/
char *G_GenerateLocalTargetName( void )
{
	static char localTargetName[16];

	Q_snprintfz( localTargetName, sizeof( localTargetName ), "localtargetmnbvcxz_%i", level.numLocalTargetNames );
	level.numLocalTargetNames++;
	return localTargetName;
}

/*
* G_Target_Delayer_Think
*/
static void G_Target_Delayer_Think( gentity_t *delayer )
{
	gentity_t *target, *trigger;

	target = delayer->owner;
	if( target->s.local.inuse && target->activate )
	{
		trigger = ( delayer->count >= 0 ) ? &game.entities[delayer->count] : NULL;
		target->activate( target, trigger, delayer->mover.activator );
	}

	G_FreeEntity( delayer );
}

/*
* G_ActivateTargets
*/
void G_ActivateTargets( gentity_t *ent, gentity_t *activator )
{
	gentity_t *target;

	if( ent->s.local.inuse && ent->target && ent->target[0] )
	{
		target = NULL;
		while( ( target = G_Find( target, FOFFSET( gentity_t, targetname ), ent->target ) ) )
		{
			if( target->activate )
			{
				if( !target->delay )
				{
					target->activate( target, ent, activator );
				}
				else if( target->timestamp < level.time )
				{
					gentity_t *delayer = G_Spawn();
					delayer->owner = target;
					delayer->mover.activator = activator;
					delayer->count = ENTNUM( ent );
					delayer->think = G_Target_Delayer_Think;
					delayer->nextthink = level.time + target->delay;

					target->timestamp = level.time + target->delay;
				}

				if( !ent->s.local.inuse )
					break; // stop if the entity was freed by the target
			}
		}
	}

	// killtargets are used to remove other entities
	if( ent->s.local.inuse && ent->killtarget && ent->killtarget[0] )
	{
		target = NULL;
		while( ( target = G_Find( target, FOFFSET( gentity_t, targetname ), ent->killtarget ) ) )
		{
			if( target->s.local.inuse )
			{
				G_FreeEntity( target );
				if( !ent->s.local.inuse )
					break; // stop if the entity was freed by the target
			}
		}
	}
}

//==================================================
// SOUNDS
//==================================================

/*
* _G_SpawnSound
*/
static gentity_t *_G_SpawnSound( int channel, int soundindex, float attenuation )
{
	gentity_t *ent;

	if( attenuation <= 0.0f )
		attenuation = ATTN_GLOBAL;

	ent = G_Spawn();
	ent->netflags &= ~SVF_NOCLIENT;
	ent->s.type = ET_SOUNDEVENT;
	ent->s.skinindex = (int)( attenuation * 16.0f );
	ent->s.modelindex1 = channel;
	ent->s.sound = soundindex;
	ent->clearSnap = G_FreeEntity; // frees itself after being sent

	return ent;
}

/*
* G_Sound
*/
void G_Sound( gentity_t *owner, int channel, int soundindex, float attenuation )
{
	gentity_t *ent;

	if( !soundindex )
		return;

	if( owner == NULL || owner == worldEntity )
		attenuation = ATTN_GLOBAL;
	else if( ISEVENTENTITY( &owner->s ) ) // event entities can't be owner of sound entities
		return;

	ent = _G_SpawnSound( channel, soundindex, attenuation );
	
	if( ent->s.skinindex != ATTN_GLOBAL )
	{
		assert( owner );
		ent->s.modelindex2 = owner->s.number;
		GS_CenterOfEntity( owner->s.ms.origin, &owner->s, ent->s.ms.origin );
	}

	GClip_LinkEntity( ent );
}

/*
* G_PositionedSound
*/
void G_PositionedSound( vec3_t origin, int channel, int soundindex, float attenuation )
{
	gentity_t *ent;

	if( !soundindex )
		return;

	if( origin == NULL )
		attenuation = ATTN_GLOBAL;

	ent = _G_SpawnSound( channel, soundindex, attenuation );
	
	if( ent->s.skinindex != ATTN_GLOBAL )
	{
		assert( origin );
		VectorCopy( origin, ent->s.ms.origin );
	}

	GClip_LinkEntity( ent );
}

/*
* G_GlobalSound
*/
void G_GlobalSound( int channel, int soundindex )
{
	G_PositionedSound( NULL, channel, soundindex, ATTN_GLOBAL );
}

/*
* G_AnnouncerSound
*/
void G_AnnouncerSound( gentity_t *target, int soundindex, int team, qboolean queued, gentity_t *ignore )
{
	int psev = queued ? PSEV_ANNOUNCER_QUEUED : PSEV_ANNOUNCER;

	if( target ) // only for a given player
	{
		if( !target->client || trap_GetClientState( ENTNUM( target ) ) < CS_SPAWNED )
			return;

		if( target == ignore )
			return;

		G_AddPlayerStateEvent( target->client, psev, soundindex );
	}
	else // add it to all players
	{
		gentity_t *ent;

		for( ent = game.entities; ENTNUM( ent ) < gs.maxclients; ent++ )
		{
			if( !ent->s.local.inuse || trap_GetClientState( ENTNUM( ent ) ) < CS_SPAWNED )
				continue;

			if( ent == ignore )
				continue;

			// team filter
			if( team >= TEAM_NOTEAM && team < GS_NUMTEAMS )
			{
				if( ent->s.team != team )
					continue;
			}

			G_AddPlayerStateEvent( ent->client, psev, soundindex );
		}
	}
}

//==================================================
// EVENTS
//==================================================

/*
* G_PredictedEvent
*/
void G_PredictedEvent( int entNum, int ev, int parm )
{
	switch( ev )
	{
	case EV_WEAPONACTIVATE:
		game.entities[entNum].s.weapon = parm;
		G_AddEvent( &game.entities[entNum], ev, parm, qtrue );
		break;
	case EV_FALLIMPACT:
		G_TakeFallDamage( &game.entities[entNum], parm );
		G_AddEvent( &game.entities[entNum], ev, parm, qtrue );
		break;
	case EV_FIREWEAPON:
		G_FireWeapon( &game.entities[entNum] );
		G_AddEvent( &game.entities[entNum], ev, parm, qtrue );
		break;
	case EV_SMOOTHREFIREWEAPON:
		G_FireWeapon( &game.entities[entNum] );
		G_AddEvent( &game.entities[entNum], ev, parm, qtrue );
		break;
	case EV_ACTIVATE:
		// this event is not forwarded to the client. No need. Also, the target number wouldn't fit into a byte.
		G_Client_Activate( &game.entities[entNum], parm );
		break;
	default:
		G_AddEvent( &game.entities[entNum], ev, parm, qtrue );
		break;
	}
}

/*
* G_ReleasePlayerStateEvent
*/
void G_ReleasePlayerStateEvent( gclient_t *client )
{
	int i;

	if( client )
	{
		for( i = 0; i < 2; i++ )
		{
			if( client->level.eventsCurrent < client->level.eventsHead )
			{
				client->ps.event[i] = client->level.events[client->level.eventsCurrent&MAX_CLIENT_EVENTS_MASK] & 127;
				client->ps.eventParm[i] = (client->level.events[client->level.eventsCurrent&MAX_CLIENT_EVENTS_MASK]>>8) & 0xFF;
				client->level.eventsCurrent++;
			}
			else
			{
				client->ps.event[i] = PSEV_NONE;
				client->ps.eventParm[i] = 0;
			}
		}
	}
}

/*
* G_AddPlayerStateEvent
* This event is only sent to this client inside its player_state_t.
*/
void G_AddPlayerStateEvent( gclient_t *client, int event, int parm )
{
	int eventdata;
	if( client )
	{
		if( !event || event > PSEV_MAX_EVENTS || parm > 0xFF )
			return;
		if( client )
		{
			eventdata = ( ( event &0xFF )|( parm&0xFF )<<8 );
			client->level.events[client->level.eventsHead&MAX_CLIENT_EVENTS_MASK] = eventdata;
			client->level.eventsHead++;
		}
	}
}

/*
* G_ClearPlayerStateEvents
*/
void G_ClearPlayerStateEvents( gclient_t *client )
{
	if( client )
	{
		memset( client->level.events, PSEV_NONE, sizeof( client->level.events ) );
		client->level.eventsCurrent = client->level.eventsHead = 0;
	}
}

//==================================================
// Configstring Indexing
//==================================================

/*
* G_ConfigstringIndex
*/
int G_ConfigstringIndex( const char *newString, int initial, int max )
{
	int i;
	const char *cstring;

	if( !newString || !newString[0] )
		return 0;

	for( i = 1; i < max; i++ )
	{
		cstring = trap_GetConfigString( initial + i );
		if( !cstring )
			GS_Error( "G_ConfigstringIndex: bad configstring requested\n" );

		if( !cstring[0] )
			break; // first free spot

		if( !Q_stricmp( cstring, newString ) )
			return i;
	}

	if( i == max )
		return -1;

	// index it
	trap_ConfigString( initial + i, newString );
	return i;
}

/*
* G_PlayerObjectIndex
*/
int G_PlayerObjectIndex( const char *name )
{
	int i;
	char string[MAX_STRING_CHARS];

	if( !name || !name[0] )
		return 0;

	if( name[0] != '#' )
	{
		GS_Printf( "WARNING: G_PlayerObjectIndex: bad object name %s\n", name );
		return 0;
	}

	if( strlen( name ) >= MAX_CONFIGSTRING_CHARS - 1 )
		GS_Error( "G_PlayerObjectIndex: Too long playerObject name: %s\n", name );

	i = G_ConfigstringIndex( name, CS_PLAYEROBJECTS, MAX_PLAYEROBJECTS );
	if( i == -1 )
		GS_Error( "G_PlayerObjectIndex: MAX_PLAYEROBJECTS reached\n" );

	// index the actual models so they are added to the auto download process
	if( trap_FS_FOpenFile( string, NULL, FS_READ ) != -1 )
	{
		Q_snprintfz( string, sizeof( string ), "%s%s/tris%s", GS_PlayerObjects_BasePath(), name+1, GS_PlayerObjects_Extension() );
		if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
			GS_Error( "G_PlayerObjectIndex: model path is too large for a configstring: %s\n", string );
		
		trap_ModelIndex( string );
		trap_PureModel( string );
	}

	return i;
}

/*
* G_WeaponObjectIndex 
* Register a weapon object name in a configstring,
* and add all the submodels to the precache
*/
int G_WeaponObjectIndex( const char *name )
{
	int i;
	char string[MAX_STRING_CHARS];

	if( !name || !name[0] )
		return 0;

	if( name[0] != '#' )
	{
		GS_Printf( "WARNING: G_WeaponObjectIndex: bad object name %s\n", name );
		return 0;
	}

	if( strlen( name ) >= MAX_CONFIGSTRING_CHARS - 1 )
		GS_Error( "G_WeaponObjectIndex: Too long weaponObject name: %s\n", name );

	i = G_ConfigstringIndex( name, CS_WEAPONOBJECTS, MAX_WEAPONOBJECTS );
	if( i == -1 )
		GS_Error( "G_WeaponObjectIndex: MAX_WEAPONOBJECTS reached\n" );

	// main model
	Q_snprintfz( string, sizeof( string ), "%s%s/weapon%s", GS_WeaponObjects_BasePath(), name+1, GS_WeaponObjects_Extension() );

	if( trap_FS_FOpenFile( string, NULL, FS_READ ) != -1 )
	{
		if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
			GS_Error( "G_WeaponObjectIndex: model path is too large for a configstring: %s\n", string );

		trap_ModelIndex( string );
		trap_PureModel( string );
	}

	// index all the models so they are added to the auto download process

	Q_snprintfz( string, sizeof( string ), "%s%s/barrel%s", GS_WeaponObjects_BasePath(), name+1, GS_WeaponObjects_Extension() );
	if( trap_FS_FOpenFile( string, NULL, FS_READ ) != -1 )
	{
		if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
			GS_Error( "G_WeaponObjectIndex: model path is too large for a configstring: %s\n", string );

		trap_ModelIndex( string );
		trap_PureModel( string );
	}

	Q_snprintfz( string, sizeof( string ), "%s%s/flash%s", GS_WeaponObjects_BasePath(), name+1, GS_WeaponObjects_Extension() );
	if( trap_FS_FOpenFile( string, NULL, FS_READ ) != -1 )
	{
		if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
			GS_Error( "G_WeaponObjectIndex: model path is too large for a configstring: %s\n", string );

		trap_ModelIndex( string );
		trap_PureModel( string );
	}

	Q_snprintfz( string, sizeof( string ), "%s%s/hand%s", GS_WeaponObjects_BasePath(), name+1, GS_WeaponObjects_Extension() );
	if( trap_FS_FOpenFile( string, NULL, FS_READ ) != -1 )
	{
		if( strlen( string ) >= MAX_CONFIGSTRING_CHARS - 1 )
			GS_Error( "G_WeaponObjectIndex: model path is too large for a configstring: %s\n", string );

		trap_ModelIndex( string );
		trap_PureModel( string );
	}

	return i;
}
