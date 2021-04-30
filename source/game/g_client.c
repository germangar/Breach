/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

/*
* G_ClientConnect
* client state is CS_FREE during this function, and the connection can be denied by returning false.
* It will be changed to CS_CONNECTED at the return, and to CS_SPAWNED shortly after that.
* Next game code function to be called is G_ClientBegin.
*/
qboolean G_ClientConnect( int clientNum, char *userinfo, qboolean fakeClient )
{
	gentity_t *ent;
	char *key;

	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "G_ClientConnect: Bad entity number" );

	if( ( trap_GetClientState( clientNum ) != CS_FREE ) && ( trap_GetClientState( clientNum ) != CS_ZOMBIE ) )
		GS_Error( "G_ClientConnect: called for an invalid client state\n" );

	// see if there are server bans

	key = Info_ValueForKey( userinfo, "ip" );
	if( SV_FilterPacket( key ) )
	{
		Info_SetValueForKey( userinfo, "rejtype", va( "%i", DROP_TYPE_GENERAL ) );
		Info_SetValueForKey( userinfo, "rejflag", va( "%i", 0 ) );
		Info_SetValueForKey( userinfo, "rejmsg", "You're ip is banned from this server" );
		return qfalse;
	}

	// if server has a password

	if( ( *password->string ) && !fakeClient )
	{
		key  = Info_ValueForKey( userinfo, "password" );
		if( strcmp( password->string, key ) )
		{
			Info_SetValueForKey( userinfo, "rejtype", va( "%i", DROP_TYPE_PASSWORD ) );
			Info_SetValueForKey( userinfo, "rejflag", va( "%i", 0 ) );
			if( key && strlen( key ) > 0 )
				Info_SetValueForKey( userinfo, "rejmsg", "Password incorrect" );
			else
				Info_SetValueForKey( userinfo, "rejmsg", "Password required" );
			return qfalse;
		}
	}

	// connection is accepted

	ent = game.entities + clientNum;
	G_InitEntity( ent );
	ent->classname = ( fakeClient ? "fakeclient" : "client" );
	ent->netflags = SVF_NOCLIENT;

	ent->client = game.clients + clientNum;
	memset( ent->client, 0, sizeof( gclient_t ) );
	ent->client->timeStamp = level.time;
	ent->client->fakeClient = fakeClient;
	ent->client->team = TEAM_SPECTATOR;

	G_ClientUserinfoChanged( clientNum, userinfo );

	// announce the new connection to server and clients
	GS_Printf( "%s%s connected from %s\n", ent->client->netname, S_COLOR_WHITE, Info_ValueForKey( userinfo, "ip" ) );
	G_PrintMsg( NULL, "%s%s connected", ent->client->netname, S_COLOR_WHITE );

	return qtrue;
}

/*
* G_ClientDisconnect
*/
void G_ClientDisconnect( int clientNum )
{
	gentity_t *ent;

	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "G_ClientDisconnect: Bad entity number" );

	ent = &game.entities[clientNum];

	if( !ent->client || !ent->s.local.inuse )
		return;

	// clear up the old data
	memset( ent->client, 0, sizeof( gclient_t ) );
	G_InitEntity( ent );
	GClip_UnlinkEntity( ent );
	ent->s.local.inuse = qfalse;

	// let the client know it is free
	trap_ConfigString( CS_PLAYERINFOS + clientNum, "" );
}

/*
* G_ClientUpdateInfoString
*/
void G_ClientUpdateInfoString( gclient_t *client )
{
	char cstring[MAX_INFO_STRING];

	if( !client )
		return;

	memset( cstring, 0, sizeof( cstring ) );

	if( trap_GetClientState( CLIENTNUM( client ) ) != CS_FREE )
	{
		if( !Info_SetValueForKey( cstring, "name", client->netname ) )
			GS_Printf( "WARNING: G_ClientUpdateInfoString: Client %s. Not space enough for key 'name'\n", client->netname );

		if( !Info_SetValueForKey( cstring, "hand", va( "%i", client->hand ) ) )
			GS_Printf( "WARNING: G_ClientUpdateInfoString: Client %s. Not space enough for key 'hand'\n", client->netname );

		if( !Info_SetValueForKey( cstring, "fov", va( "%i", client->fov ) ) )
			GS_Printf( "WARNING: G_ClientUpdateInfoString: Client %s. Not space enough for key 'fov'\n", client->netname );

		if( !Info_SetValueForKey( cstring, "zfov", va( "%i", client->zoomfov ) ) )
			GS_Printf( "WARNING: G_ClientUpdateInfoString: Client %s. Not space enough for key 'zfov'\n", client->netname );

		if( !Info_SetValueForKey( cstring, "color", va( "%i %i %i", client->color[0], client->color[1], client->color[2] ) ) )
			GS_Printf( "WARNING: G_ClientUpdateInfoString: Client %s. Not space enough for key 'color'\n", client->netname );
	}

	trap_ConfigString( CS_PLAYERINFOS + CLIENTNUM( client ), cstring );
}

/*
* G_ClientBegin
* The client enters a new level
*/
void G_ClientBegin( int clientNum )
{
	gentity_t *ent;

	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "G_ClientBegin: Bad entity number" );

	if( trap_GetClientState( clientNum ) < CS_SPAWNED )
		GS_Error( "G_ClientBegin: Begin without CS_SPAWNED\n" );

	ent = &game.entities[clientNum];

	G_PrintMsg( NULL, "%s %sentered the game\n", ent->client->netname, S_COLOR_WHITE );

	memset( &ent->client->level, 0, sizeof( client_level_t ) );
	ent->client->level.timeStamp = level.time;

	// remove player class
	ent->client->playerClassIndex = 0;

	G_ClientUpdateInfoString( ent->client );

	// keep him temporary as ghost and add him to spawn queue
	G_Client_Respawn( ent, qtrue );
	G_SpawnQueue_AddClient( ent );
}

/*
* G_ClientUserinfoChanged
* The server lets us know the client has changed one of his userinfo cvars.
*/
void G_ClientUserinfoChanged( int clientNum, char *userinfo )
{
	char *key;
	gentity_t *ent;
	int rgbcolor;
	gclient_t *client;

	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "G_ClientUserinfoChanged: Bad entity number" );

	if( !Info_Validate( userinfo ) )
	{
		trap_DropClient( clientNum, DROP_TYPE_GENERAL, "Invalid userinfo string\n" );
		return;
	}

	ent = &game.entities[clientNum];
	client = ent->client;

	// name
	key = Info_ValueForKey( userinfo, "name" );
	if( !key || !strlen( key ) )
	{
		// TODO: strict validation of the name
		key = "bad_name";
		Info_SetValueForKey( userinfo, "name", key );
	}
	if( trap_GetClientState( clientNum ) >= CS_SPAWNED && Q_stricmp( key, client->netname ) )
	{                                                                                     // warn name changes
		G_PrintMsg( NULL, "%s%s renames to %s%s\n", client->netname, S_COLOR_WHITE, key, S_COLOR_WHITE );
	}
	Q_strncpyz( client->netname, key, sizeof( client->netname ) );

	// hand
	key = Info_ValueForKey( userinfo, "hand" );
	if( !key || !strlen( key ) )
	{
		key = "0";
		Info_SetValueForKey( userinfo, "hand", key );
	}
	client->hand = atoi( key );

	// color
	rgbcolor = COM_ReadColorRGBString( Info_ValueForKey( userinfo, "color" ) );
	if( rgbcolor == -1 )
	{
		G_PrintMsg( ent->client, "Warning: Bad 'color' cvar values. Using white\n" );
		rgbcolor = COLOR_RGB( 255, 255, 255 );
	}
	Vector4Set( client->color, COLOR_R( rgbcolor ), COLOR_G( rgbcolor ), COLOR_B( rgbcolor ), 255 );

	// fov
	key = Info_ValueForKey( userinfo, "fov" );
	if( !key || !strlen( key ) )
	{
		key = "90";
		Info_SetValueForKey( userinfo, "fov", key );
	}
	client->fov = atoi( key );
	clamp( client->fov, 1, 160 );

	// zoomfov
	key = Info_ValueForKey( userinfo, "zoomfov" );
	if( !key || !strlen( key ) )
	{
		key = "40";
		Info_SetValueForKey( userinfo, "zoomfov", key );
	}
	client->zoomfov = atoi( key );
	clamp( client->zoomfov, 1, 90 );

	// update the client with the new info
	G_ClientUpdateInfoString( client );

	// these are managed in the server and no need of updating into the client

#ifdef UCMDTIMENUDGE
	client->ucmdTimeNudge = atoi( Info_ValueForKey( userinfo, "cl_ucmdTimeNudge" ) );
	if( client->ucmdTimeNudge < -MAX_UCMD_TIMENUDGE )
		client->ucmdTimeNudge = -MAX_UCMD_TIMENUDGE;
	else if( client->ucmdTimeNudge > MAX_UCMD_TIMENUDGE )
		client->ucmdTimeNudge = MAX_UCMD_TIMENUDGE;
#endif
}

/*
* G_Client_SpawnFakeClient
*/
gentity_t *G_SpawnFakeClient( const char *name )
{
	char userinfo[MAX_INFO_STRING];
	static char fakeIP[] = "127.0.0.1";
	int entNum, i, count;

	userinfo[0] = 0;
	count = 0;

	if( name && name[0] )
	{
		Info_SetValueForKey( userinfo, "name", name );
	}
	else
	{
		for( i = 0; i < gs.maxclients; i++ )
		{
			if( trap_GetClientState( i ) < CS_CONNECTING )
				continue;

			if( game.clients[i].fakeClient )
				count++;
		}

		Info_SetValueForKey( userinfo, "name", va( "fakeclient_%i\n", count ) );
	}

	Info_SetValueForKey( userinfo, "hand", "0" );

	// spawn it
	entNum = trap_FakeClientConnect( userinfo, fakeIP );
	if( entNum == ENTITY_INVALID )
		return NULL;

	return &game.entities[ entNum ];
}

/*
* G_Client_Spawn - It handles the case of ghosting (spectate) too.
*/
static void G_Client_Spawn( gentity_t *ent, vec3_t spawn_origin, vec3_t spawn_angles, qboolean ghost )
{
	gclient_t *client = ent->client;
	gsplayerclass_t	*playerClass;

	playerClass = GS_PlayerClassByIndex( client->playerClassIndex );

	if( ent->client->team == TEAM_SPECTATOR )
		ghost = qtrue;

	// clear up client_spawn_t struct
	memset( &client->spawn, 0, sizeof( client_spawn_t ) );
	client->spawn.timeStamp = level.time;

	// clear playerState
	memset( &client->ps, 0, sizeof( client->ps ) );

	// always ghost the entity
	GClip_UnlinkEntity( ent );

	if( ghost )
	{
		G_InitEntity( ent );
		ent->s.flags |= SFL_TELEPORTED;
		ent->netflags &= ~SVF_NOCLIENT;
		ent->s.ms.type = MOVE_TYPE_FREEFLY;
		VectorCopy( spawn_origin, ent->s.ms.origin );
		VectorCopy( spawn_angles, ent->s.ms.angles );
	}
	else
	{  // spawn a player model
		G_et_player_spawn( ent, spawn_origin, spawn_angles, NULL );
	}

	ent->classname = "client";

	// both ghosts and active players
	ent->health = ent->maxHealth = playerClass->maxHealth;
	ent->s.team = ent->client->team;

	ent->s.playerclass = playerClass->index;
	VectorCopy( playerClass->mins, ent->s.local.mins );
	VectorCopy( playerClass->maxs, ent->s.local.maxs );
	ent->health = playerClass->health;
	ent->maxHealth = playerClass->maxHealth;

	// freeze for a moment
	client->ps.controlTimers[USERINPUT_STAT_NOUSERCONTROL] = 250;
	ent->client->ps.viewHeight = PlayerViewHeightFromBox( ent->s.local.mins, ent->s.local.maxs );

	if( !ghost )
	{
		// give initial items
		G_Item_AddToInventory( ent, GS_FindItemByIndex( WEAP_MACHINEGUN ), 0 );

		// todo: proper item set for each class
		if( !Q_stricmp( playerClass->classname, "sniper" ) )
		{
			G_Item_AddToInventory( ent, GS_FindItemByIndex( WEAP_SNIPERRIFLE ), GS_FindItemByIndex( AMMO_CELLS )->count );
			G_Item_AddToInventory( ent, GS_FindItemByIndex( AMMO_CELLS ), GS_FindItemByIndex( AMMO_CELLS )->count * 4 );
			G_Item_AddToInventory( ent, GS_FindItemByIndex( WEAP_GRENADELAUNCHER ), 0 );
		}
		else if( !Q_stricmp( playerClass->classname, "engineer" ) )
		{
			G_Item_AddToInventory( ent, GS_FindItemByIndex( WEAP_LASERGUN ), GS_FindItemByIndex( AMMO_CELLS )->count );
			G_Item_AddToInventory( ent, GS_FindItemByIndex( AMMO_CELLS ), GS_FindItemByIndex( AMMO_CELLS )->count * 4 );
			G_Item_AddToInventory( ent, GS_FindItemByIndex( WEAP_NANOFORGE ), 0 );
		}
		else
		{
			G_Item_AddToInventory( ent, GS_FindItemByIndex( WEAP_ROCKETLAUNCHER ), GS_FindItemByIndex( AMMO_CELLS )->count );
			G_Item_AddToInventory( ent, GS_FindItemByIndex( AMMO_CELLS ), GS_FindItemByIndex( AMMO_CELLS )->count * 4 );
			G_Item_AddToInventory( ent, GS_FindItemByIndex( WEAP_GRENADELAUNCHER ), 0 );
		}

		client->ps.stats[STAT_PENDING_WEAPON] = GS_SelectBestWeapon( &client->ps );
	}

	GClip_LinkEntity( ent );
}

/*
* G_Client_Respawn
*/
qboolean G_Client_Respawn( gentity_t *ent, qboolean ghost )
{
	vec3_t spawn_origin, spawn_angles;

	if( !G_SelectClientSpawnPoint( ent, ent->client->team, spawn_origin, spawn_angles ) )
		return qfalse;

	G_Client_Spawn( ent, spawn_origin, spawn_angles, ghost );

	return qtrue;
}

/*
* G_ClientDie
*/
void G_ClientDie( gentity_t *ent )
{
	G_Client_Respawn( ent, qtrue );
	G_SpawnQueue_AddClient( ent );
}

/*
* G_ClientCloseSnap
*/
void G_ClientCloseSnap( gentity_t *ent )
{
	gclient_t *client = ent->client;

	if( trap_GetClientState( ENTNUM( ent ) ) < CS_SPAWNED )
		return;

	client->ps.POVnum = ENTNUM( ent );
	client->ps.viewType = VIEWDEF_PLAYERVIEW;

	// set basic fov if none is set
	if( client->ps.fov <= 0.0f )
		client->ps.fov = client->fov;

	// update stats
	client->ps.stats[STAT_HEALTH] = ent->health;
	client->ps.stats[STAT_WATER_BLEND] = client->spawn.snap.waterShaderIndex;
	client->ps.stats[STAT_NEXT_RESPAWN] = ceil( G_SpawnQueue_NextRespawnTime( client->team ) * 0.001f );

	// update effects
	ent->s.effects &= ~EF_BUSYICON;
	if( client->spawn.snap.buttons & BUTTON_BUSYICON )
		ent->s.effects |= EF_BUSYICON;

	// update sound
	ent->s.sound = 0;

	// add color blends from accumulated damage.
	if( ent->snap.damage_taken >= 1.0f )
	{
		G_AddPlayerStateEvent( ent->client, PSEV_DAMAGED, (int)ent->snap.damage_taken );
	}

	G_ReleasePlayerStateEvent( ent->client );
}

/*
* G_Client_Activate - The client has pressed the activate button
*/
void G_Client_Activate( gentity_t *ent, int targetNum )
{
	if( targetNum != ENTITY_INVALID )
	{
		gentity_t *target;

		target = &game.entities[targetNum];
		if( target->s.local.inuse )
		{
			if( target->s.type == ET_ITEM )
			{
				G_Item_EntityPickUp( target, ent );
			}
			else if( target->activate && !target->targetname )
				target->activate( target, ent, ent );
		}
	}
}

void G_Client_SmoothTimeDelta( gclient_t *client, int newTimeDelta )
{
	int nudge, i, count, delta;
	int fixedNudge = ( gs.snapFrameTime ) * 0.5; // fixme: find where this nudge comes from.

	if( client->fakeClient )
	{
		client->timeDelta = 0;
		return;
	}

	// add smoothing to timeDelta between the last few ucmds and a small fine-tuning nudge.
	nudge = fixedNudge + g_antilag_timenudge->integer;
	newTimeDelta += nudge;
	clamp( newTimeDelta, -g_antilag_maxtimedelta->integer, 0 );

	// smooth using last valid deltas
	i = client->timeDeltasHead - 6;
	if( i < 0 ) i = 0;
	for( count = 0, delta = 0; i < client->timeDeltasHead; i++ )
	{
		if( client->timeDeltas[i & G_MAX_TIME_DELTAS_MASK] < 0 )
		{
			delta += client->timeDeltas[i & G_MAX_TIME_DELTAS_MASK];
			count++;
		}
	}

	if( !count )
		client->timeDelta = newTimeDelta;
	else
	{
		delta /= count;
		client->timeDelta = ( delta + newTimeDelta ) * 0.5;
	}

	client->timeDeltas[client->timeDeltasHead & G_MAX_TIME_DELTAS_MASK] = newTimeDelta;
	client->timeDeltasHead++;

#ifdef UCMDTIMENUDGE
	client->timeDelta += client->pers.ucmdTimeNudge;
#endif

	clamp( client->timeDelta, -g_antilag_maxtimedelta->integer, 0 );
}

/*
* G_ClientThink
*/
void G_ClientThink( int clientNum, usercmd_t *ucmd, int timeDelta )
{
	gentity_t *ent;
	gclient_t *client;
	static move_t move;
	int contentMask;
	vec3_t newAccel;

	if( clientNum < 0 || clientNum >= gs.maxclients )
		GS_Error( "ClientThink: Bad entity number" );

	ent = &game.entities[clientNum];
	client = ent->client;

	client->spawn.snap.buttons |= ucmd->buttons;

	G_Client_SmoothTimeDelta( client, timeDelta );

	VectorCopy( ucmd->angles, client->cmd_angles );
	client->ucmdTime = ucmd->serverTimeStamp;

	contentMask = GS_ContentMaskForState( &ent->s );

	// snap initial if the position has changed since last move.
#ifdef MOVE_SNAP_ORIGIN
	if( !VectorCompare( client->level.last_entity_state.ms.origin, ent->s.ms.origin )
		|| !VectorCompare( client->level.last_entity_state.local.mins, ent->s.local.mins )
		|| !VectorCompare( client->level.last_entity_state.local.maxs, ent->s.local.maxs )
		|| ( client->level.last_entity_state.solid != ent->s.solid ) )
		GS_SnapInitialPosition( ent->s.ms.origin, ent->s.local.mins, ent->s.local.maxs, ent->s.number, contentMask );
#endif

	// modify angles and velocity from user input
	GS_Client_ApplyUserInput( newAccel, &ent->s, &client->ps, ucmd, client->fov, client->zoomfov, contentMask, client->timeDelta );

	// run physics applying the accel from user input
	G_RunPhysicsTick( ent, newAccel, ucmd->msec );

	client->level.last_entity_state = ent->s;
}
