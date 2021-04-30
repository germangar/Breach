/*
   Copyright (C) 2007 German Garcia
 */

#include "g_local.h"

//==================================================
// GENERIC TRIGGER
//==================================================

//=================
//G_Trigger_Activate
//=================
static void G_Trigger_Activate( gentity_t *trigger, gentity_t *other, gentity_t *activator )
{
	if( trigger->timestamp + trigger->wait > level.time )  // already been triggered. It's waiting the wait time
		return;

	trigger->timestamp = level.time;
	G_ActivateTargets( trigger, activator );
}

//=================
//G_Trigger_Touch
//=================
void G_Trigger_Touch( gentity_t *trigger, gentity_t *other, cplane_t *plane, int surfFlags )
{
	if( trigger->s.team && trigger->s.team != other->s.team )
		return;

	// don't let triggers be touched by their own targets
	if( G_IsTargetOfEnt( trigger, other ) )
		return;

	G_Trigger_Activate( trigger, other, other );
}

//=================
//G_Trigger_Spawn
//=================
void G_Trigger_Spawn( gentity_t *ent )
{
	GClip_SetBrushModel( ent, ent->model );

	ent->s.type = ET_TRIGGER;
	ent->s.solid = SOLID_TRIGGER;
	ent->s.ms.type = MOVE_TYPE_NONE;
	ent->netflags &= ~SVF_NOCLIENT;

	if( ent->wait < 0 )
		ent->wait = 0.2f;

	ent->wait *= 1000; // convert to milliseconds

	ent->touch = G_Trigger_Touch;
	ent->activate = G_Trigger_Activate;

	GClip_LinkEntity( ent );
}

//==================================================
// SPECIAL TRIGGERS
//==================================================

//=================
//G_Trigger_WaterBlend_Touch
//=================
static void G_Trigger_WaterBlend_Touch( gentity_t *trigger, gentity_t *other, cplane_t *plane, int surfFlags )
{
	if( other->client )
		other->client->spawn.snap.waterShaderIndex = trigger->count;
}

//=================
//G_Trigger_WaterBlend_Spawn - Never in the client
//=================
void G_Trigger_WaterBlend_Spawn( gentity_t *ent )
{
	const char *s;

	GClip_SetBrushModel( ent, ent->model );

	ent->s.type = ET_TRIGGER;
	ent->s.solid = SOLID_TRIGGER;
	ent->s.ms.type = MOVE_TYPE_NONE;
	//ent->netflags &= ~SVF_NOCLIENT;
	ent->s.team = TEAM_NOTEAM;
	ent->wait = 0;

	s = G_GetEntitySpawnKey( "shader", ent );
	if( s[0] )
		ent->count = trap_ImageIndex( s );

	if( !ent->count )
	{
		GS_Printf( "WARNING: G_Trigger_WaterBlend_Spawn: couldn't find shader %s. Inhibited\n", s );
		G_FreeEntity( ent );
		return;
	}

	ent->touch = G_Trigger_WaterBlend_Touch;

	GClip_LinkEntity( ent );
}
