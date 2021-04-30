/*
   Copyright (C) 2007 German Garcia
 */

#include "g_local.h"

//=================
//G_et_player_die
//=================
static void G_et_player_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage )
{
	if( ent->client )
	{
		G_ClientDie( ent );
	}
	else
	{
		G_FreeEntity( ent );
	}
}

//=================
//G_et_player_die
//=================
static void G_et_player_pain( gentity_t *ent, gentity_t *other, float kick, float damage )
{
}

//=================
//G_et_player_think
//=================
static void G_et_player_think( gentity_t *ent )
{
}

//=================
//G_et_player_closeSnap
//=================
static void G_et_player_closeSnap( gentity_t *ent )
{
	// add the damage effects
	if( ent->snap.damage_taken > 2 )
	{
		if( ent->snap.damage_taken >= 50 )
		{
			G_AddEvent( ent, EV_PAIN_STRONG, 0, qtrue );
			ent->paintimestamp = level.time;
		}
		else if( ent->snap.damage_taken > 15 && ent->paintimestamp + 200 < level.time  )
		{
			G_AddEvent( ent, EV_PAIN_MEDIUM, 0, qtrue );
			ent->paintimestamp = level.time;
		}
		else if( ent->paintimestamp + 400 < level.time )
		{
			G_AddEvent( ent, EV_PAIN_SOFT, 0, qtrue );
			ent->paintimestamp = level.time;
		}
	}
}

//=================
//G_et_player_clearSnap
//=================
static void G_et_player_clearSnap( gentity_t *ent )
{
	ent->s.effects = 0;
}

//=================
//G_et_player_spawn
//=================
void G_et_player_spawn( gentity_t *ent, vec3_t spawn_origin, vec3_t spawn_angles, char *playerObject )
{
	GClip_UnlinkEntity( ent );

	G_InitEntity( ent );
	ent->netflags &= ~SVF_NOCLIENT;
	ent->s.flags = SFL_TAKEDAMAGE|SFL_TELEPORTED;

	ent->s.type = ET_PLAYER;
	ent->s.ms.type = MOVE_TYPE_STEP;
	ent->s.solid = SOLID_PLAYER;

	ent->think = G_et_player_think;
	ent->pain = G_et_player_pain;
	ent->die = G_et_player_die;
	ent->activate = NULL;
	ent->touch = NULL;
	ent->clearSnap = G_et_player_clearSnap;
	ent->closeSnap = G_et_player_closeSnap;

	ent->mass = 200;

	// set new position
	VectorCopy( spawn_origin, ent->s.ms.origin );
	VectorCopy( spawn_angles, ent->s.ms.angles );
	VectorClear( ent->s.origin2 );

	// set the class info if any
	if( playerObject )
		ent->s.modelindex1 = G_PlayerObjectIndex( playerObject );
	VectorCopy( GS_PlayerClassByIndex( 0 )->mins, ent->s.local.mins );
	VectorCopy( GS_PlayerClassByIndex( 0 )->maxs, ent->s.local.maxs );
	ent->health = ent->maxHealth = 100;

	// link in the world
	ent->s.cmodeltype = CMODEL_BBOX;
	if( ent->s.solid != SOLID_NOT )
	{
		if( !KillBox( ent ) )
		{
		}
	}

	GClip_LinkEntity( ent );
}
