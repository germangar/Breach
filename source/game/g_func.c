/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

//==================================================
//
// FUNC ENTITIES
//
//==================================================

void G_Mover_Activate( gentity_t *ent, gentity_t *other, gentity_t *activator );
void G_Mover_Blocked( gentity_t *ent, gentity_t *other );
void G_Mover_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage );

void G_Rotating_Activate( gentity_t *ent, gentity_t *other, gentity_t *activator );
void G_Rotating_Blocked( gentity_t *ent, gentity_t *other );

void G_Trigger_Touch( gentity_t *trigger, gentity_t *other, cplane_t *plane, int surfFlags );

//==================================================
// FUNC PLATS
//==================================================

/*
static void Plat_Ride( gentity_t *ent, gentity_t *other, cplane_t *plane, int surfFlags ) {
if( (other->env.groundentity == ent->s.number) && (ent->mover.state == STATE_ENDPOS) ) {
ent->nextthink = level.time + 3000;
}
}*/

/*
* G_Plat_SpawnTrigger
*/
static void G_Plat_SpawnTrigger( gentity_t *ent, qboolean bottom_trigger )
{
	gentity_t *trigger;
	struct cmodel_s *cmodel;
	float expand_size = -24;

	trigger = G_Spawn();
	trigger->s.type = ET_TRIGGER;
	trigger->s.team = ent->s.team;
	trigger->s.ms.type = MOVE_TYPE_NONE;
	trigger->s.solid = SOLID_TRIGGER;
	trigger->s.cmodeltype = CMODEL_BBOX;
	VectorCopy( ent->s.ms.origin, trigger->s.ms.origin );
	trigger->classname = "plat_auto_trigger";
	trigger->netflags &= ~SVF_NOCLIENT;
	trigger->target = ent->targetname;
	trigger->touch = G_Trigger_Touch;
	if( !trigger->target )
		GS_Printf( "WARNING: func_plat auto-trigger without a target\n" );

	cmodel = GS_CModelForEntity( &ent->s );
	if( !cmodel )
		GS_Error( "G_Plat_SpawnTrigger: Plat entity without a brush model. solid %i cmodeltype %i\n", ent->s.solid, ent->s.cmodeltype );

	GS_RelativeBoxForBrushModel( cmodel, trigger->s.ms.origin, trigger->s.local.mins, trigger->s.local.maxs );

	// expand
	trigger->s.local.mins[0] -= expand_size;
	trigger->s.local.mins[1] -= expand_size;

	trigger->s.local.maxs[0] += expand_size;
	trigger->s.local.maxs[1] += expand_size;

	trigger->s.local.mins[2] += st.lip;

	if( bottom_trigger )
	{
		// encoded bboxes have their limits, we must move the origin for making this properly
		if( ent->spawnflags & MOVER_FLAG_REVERSE )
		{
			trigger->s.ms.origin[2] = trigger->s.ms.origin[2] + trigger->s.local.maxs[2];
			trigger->s.local.mins[2] = 0;
			trigger->s.local.maxs[2] = 16;
		}
		else
		{
			trigger->s.ms.origin[2] = trigger->s.ms.origin[2] + trigger->s.local.mins[2];
			trigger->s.local.mins[2] = 0;
			trigger->s.local.maxs[2] = 16;
		}
	}
	else
	{
		trigger->s.local.maxs[2] += 8;
		trigger->s.local.mins[2] -= 8;
	}

	GClip_LinkEntity( trigger );
}

/*
* G_Func_Plat
*/
void G_Func_Plat( gentity_t *ent )
{
	qboolean bottom_trigger = qtrue;

	G_InitMover( ent );

	ent->mover.wait = ent->wait * 1000;
	if( ent->mover.wait == 0 && !( ent->spawnflags & MOVER_FLAG_TOGGLE ) )
		ent->mover.wait = 2000;

	if( st.speed > 0 )
		ent->mover.speed = st.speed;
	if( !ent->mover.speed )
		ent->mover.speed = 300;

	if( !st.lip )
		st.lip = 8;

	if( ent->delay < 60 )
		ent->delay = 60;

	ent->mover.sound = G_LocalSound( SOUND_PLAT_MOVE );
	ent->mover.event_endpos = EV_PLAT_STOP;
	ent->mover.event_startpos = EV_PLAT_STOP;
	ent->mover.event_startmoving = EV_PLAT_START;
	ent->mover.event_startreturning = EV_PLAT_START;

	ent->activate = G_Mover_Activate;
	ent->blocked = G_Mover_Blocked;
	//ent->touch = Plat_Ride;
	ent->think = NULL;
	ent->die = G_Mover_Die;
	ent->pain = NULL;

	// start is the top position, end is the bottom
	VectorCopy( ent->s.ms.origin, ent->mover.start_origin );
	VectorCopy( ent->s.ms.origin, ent->mover.end_origin );
	if( st.height )
		ent->mover.start_origin[2] -= st.height;
	else
		ent->mover.start_origin[2] -= ( ent->s.local.maxs[2] - ent->s.local.mins[2] ) - st.lip;

	VectorClear( ent->s.ms.angles );
	VectorCopy( ent->s.ms.angles, ent->mover.start_angles );
	VectorCopy( ent->s.ms.angles, ent->mover.end_angles );

	if( !ent->targetname )
	{
		// it has no trigger assigned
		ent->targetname = GS_CopyString( G_GenerateLocalTargetName() );
		G_Plat_SpawnTrigger( ent, bottom_trigger ); // the "start moving" trigger
	}

	if( ent->spawnflags & MOVER_FLAG_REVERSE )
	{
		VectorCopy( ent->mover.end_origin, ent->s.ms.origin );
		VectorCopy( ent->mover.start_origin, ent->mover.end_origin );
		VectorCopy( ent->s.ms.origin, ent->mover.start_origin );
	}
	else
	{
		VectorCopy( ent->mover.start_origin, ent->s.ms.origin );
	}

	ent->mover.state = MOVER_STATE_STARTPOS;

	GClip_LinkEntity( ent );
}

//==================================================
// FUNC DOORS
//==================================================

/*
* G_Door_SpawnTrigger
*/
static void G_Door_SpawnTrigger( gentity_t *ent )
{
	gentity_t *trigger;
	struct cmodel_s *cmodel;
	float expand_size = 80;

	trigger = G_Spawn();
	trigger->s.type = ET_TRIGGER;
	trigger->s.team = ent->s.team;
	trigger->s.ms.type = MOVE_TYPE_NONE;
	trigger->s.solid = SOLID_TRIGGER;
	trigger->s.cmodeltype = CMODEL_BBOX;
	VectorCopy( ent->s.ms.origin, trigger->s.ms.origin );
	trigger->classname = "door_auto_trigger";
	trigger->netflags &= ~SVF_NOCLIENT;
	trigger->touch = G_Trigger_Touch;
	trigger->target = ent->targetname;
	if( !trigger->target )
		GS_Printf( "WARNING: func_door auto-trigger without a target\n" );

	cmodel = GS_CModelForEntity( &ent->s );
	if( !cmodel )
		GS_Error( "G_Door_SpawnTrigger: Plat entity without a brush model. solid %i cmodeltype %i\n", ent->s.solid, ent->s.cmodeltype );

	GS_RelativeBoxForBrushModel( cmodel, trigger->s.ms.origin, trigger->s.local.mins, trigger->s.local.maxs );

	// expand
	trigger->s.local.mins[0] -= expand_size;
	trigger->s.local.mins[1] -= expand_size;
	trigger->s.local.maxs[0] += expand_size;
	trigger->s.local.maxs[1] += expand_size;

	GClip_LinkEntity( trigger );
}

/*
* G_Func_Door
*/
void G_Func_Door( gentity_t *ent )
{
	struct cmodel_s *cmodel;
	vec3_t abs_movedir;
	vec3_t absmins, absmaxs, size;
	float dist;

	G_InitMover( ent );
	ent->mover.is_areaportal = qtrue;

	ent->mover.wait = ent->wait * 1000;
	if( ent->mover.wait == 0 && !( ent->spawnflags & MOVER_FLAG_TOGGLE ) )
		ent->mover.wait = 2000;

	if( st.speed > 0 )
		ent->mover.speed = st.speed;
	if( !ent->mover.speed )
		ent->mover.speed = 1200;

	if( !st.lip )
		st.lip = 8;

	if( ent->delay < 0 )
		ent->delay = 0;

	ent->mover.sound = G_LocalSound( SOUND_DOOR_MOVE );
	ent->mover.event_endpos = EV_DOOR_HIT_TOP;
	ent->mover.event_startpos = EV_DOOR_HIT_BOTTOM;
	ent->mover.event_startmoving = EV_DOOR_START_MOVING;
	ent->mover.event_startreturning = EV_DOOR_START_MOVING;

	ent->blocked = G_Mover_Blocked;
	ent->activate = G_Mover_Activate;
	ent->think = NULL;
	ent->touch = NULL;
	ent->die = G_Mover_Die;
	ent->pain = NULL;

	// FIXME: Q3 and WSW maps have door angles set as anglehack. I don't want anglehack, but
	// we have to use it until we stop testing with their maps
#if 0
	G_SetMovedir( ent->s.ms.angles, ent->mover.movedir );
#else
	G_SetMovedir( tv( 0, st.anglehack, 0 ), ent->mover.movedir );
#endif
	// calculate positions
	cmodel = GS_CModelForEntity( &ent->s );
	if( !cmodel )
		GS_Error( "G_Func_Door: entity without a brush model. solid %i cmodeltype %i\n", ent->s.solid, ent->s.cmodeltype );
	trap_CM_InlineModelBounds( cmodel, absmins, absmaxs );
	VectorSubtract( absmaxs, absmins, size );

	abs_movedir[0] = fabs( ent->mover.movedir[0] );
	abs_movedir[1] = fabs( ent->mover.movedir[1] );
	abs_movedir[2] = fabs( ent->mover.movedir[2] );
	dist = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - st.lip;

	VectorCopy( ent->s.ms.origin, ent->mover.start_origin );
	VectorMA( ent->mover.start_origin, dist, ent->mover.movedir, ent->mover.end_origin );

	VectorClear( ent->s.ms.angles );
	VectorCopy( ent->s.ms.angles, ent->mover.start_angles );
	VectorCopy( ent->s.ms.angles, ent->mover.end_angles );

	if( !ent->targetname )
	{
		// it has no trigger assigned
		ent->targetname = GS_CopyString( G_GenerateLocalTargetName() );
		G_Door_SpawnTrigger( ent );
	}

	if( ent->spawnflags & MOVER_FLAG_REVERSE )
	{
		VectorCopy( ent->mover.end_origin, ent->s.ms.origin );
		VectorCopy( ent->mover.start_origin, ent->mover.end_origin );
		VectorCopy( ent->s.ms.origin, ent->mover.start_origin );
		VectorNegate( ent->mover.movedir, ent->mover.movedir );
	}

	ent->mover.state = MOVER_STATE_STARTPOS;
	G_Door_SetAreaportalState( ent, ( ent->spawnflags & MOVER_FLAG_REVERSE ) ? qtrue : qfalse );

	GClip_LinkEntity( ent );
}

/*
* G_Func_Door_Rotating
*/
void G_Func_Door_Rotating( gentity_t *ent )
{
	float dist;

	G_InitMover( ent );
	ent->mover.rotating = qtrue;
	ent->mover.is_areaportal = qtrue;

	ent->mover.wait = ent->wait * 1000;
	if( ent->mover.wait == 0 && !( ent->spawnflags & MOVER_FLAG_TOGGLE ) )
		ent->mover.wait = 2000;

	if( st.speed > 0 )
		ent->mover.speed = st.speed;
	if( !ent->mover.speed )
		ent->mover.speed = 300;

	if( ent->delay < 0 )
		ent->delay = 0;

	ent->mover.sound = G_LocalSound( SOUND_DOOR_MOVE );
	ent->mover.event_endpos = EV_DOOR_HIT_TOP;
	ent->mover.event_startpos = EV_DOOR_HIT_BOTTOM;
	ent->mover.event_startmoving = EV_DOOR_START_MOVING;
	ent->mover.event_startreturning = EV_DOOR_START_MOVING;

	ent->blocked = G_Mover_Blocked;
	ent->activate = G_Mover_Activate;
	ent->think = NULL;
	ent->touch = NULL;
	ent->die = G_Mover_Die;
	ent->pain = NULL;

	G_RotationFromAngles( ent->s.ms.angles, ent->mover.movedir );
	VectorClear( ent->s.ms.angles );

	if( !st.distance )
	{
		if( developer->integer )
			GS_Printf( "%s at %s with no distance set\n", ent->classname, vtos( ent->s.ms.origin ) );
		st.distance = 90;
	}

	dist = st.distance;
	VectorCopy( ent->s.ms.angles, ent->mover.start_angles );
	VectorMA( ent->mover.start_angles, dist, ent->mover.movedir, ent->mover.end_angles );


	if( !ent->targetname )
	{
		// it has no trigger assigned
		ent->targetname = GS_CopyString( G_GenerateLocalTargetName() );
		G_Door_SpawnTrigger( ent );
	}

	// if it starts open, switch the positions
	if( ent->spawnflags & MOVER_FLAG_REVERSE )
	{
		VectorCopy( ent->mover.end_angles, ent->s.ms.angles );
		VectorCopy( ent->mover.start_angles, ent->mover.end_angles );
		VectorCopy( ent->s.ms.angles, ent->mover.start_angles );
		VectorNegate( ent->mover.movedir, ent->mover.movedir );
	}

	ent->mover.state = MOVER_STATE_STARTPOS;
	VectorCopy( ent->s.ms.origin, ent->mover.start_origin );
	VectorCopy( ent->s.ms.origin, ent->mover.end_origin );

	G_Door_SetAreaportalState( ent, ( ent->spawnflags & MOVER_FLAG_REVERSE ) ? qtrue : qfalse );

	GClip_LinkEntity( ent );
}

//==================================================
// FUNC BUTTONS
//==================================================

/*
* G_Button_Touch
*/
static void G_Button_Touch( gentity_t *ent, gentity_t *other, cplane_t *plane, int surfFlags )
{
	gentity_t *activator;

	activator = other;
	if( other->s.solid == SOLID_PROJECTILE )
	{
		if( !( ent->spawnflags & MOVER_FLAG_ALLOW_SHOT_ACTIVATION ) )
			return;

		activator = other->owner;
	}

	if( ent->spawnflags & MOVER_FLAG_DENY_TOUCH_ACTIVATION )
		return;

	G_Mover_Activate( ent, other, activator );
}

/*
* G_Func_Button
*/
void G_Func_Button( gentity_t *ent )
{
	struct cmodel_s *cmodel;
	vec3_t absmins, absmaxs, size;
	vec3_t abs_movedir;
	float dist;

	G_InitMover( ent );

#if 1
	G_SetMovedir( ent->s.ms.angles, ent->mover.movedir );
#else
	G_SetMovedir( tv( 0, st.anglehack, 0 ), ent->mover.movedir );
#endif

	ent->mover.wait = ent->wait * 1000;
	if( st.speed > 0 )
		ent->mover.speed = st.speed;
	if( !ent->mover.speed )
		ent->mover.speed = 150;

	ent->mover.sound = 0;
	ent->mover.event_endpos = 0;
	ent->mover.event_startpos = 0;
	ent->mover.event_startmoving = EV_BUTTON_START;
	ent->mover.event_startreturning = 0;

	ent->blocked = G_Mover_Blocked;
	ent->activate = G_Mover_Activate;
	ent->think = NULL;
	ent->die = G_Mover_Die;
	ent->pain = NULL;
	if( !ent->targetname )
		ent->touch = G_Button_Touch;

	// calculate positions
	cmodel = GS_CModelForEntity( &ent->s );
	if( !cmodel )
		GS_Error( "G_Func_Button: entity without a brush model. solid %i cmodeltype %i\n", ent->s.solid, ent->s.cmodeltype );
	trap_CM_InlineModelBounds( cmodel, absmins, absmaxs );
	VectorSubtract( absmaxs, absmins, size );

	abs_movedir[0] = fabs( ent->mover.movedir[0] );
	abs_movedir[1] = fabs( ent->mover.movedir[1] );
	abs_movedir[2] = fabs( ent->mover.movedir[2] );
	dist = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - st.lip;

	VectorCopy( ent->s.ms.origin, ent->mover.start_origin );
	VectorMA( ent->mover.start_origin, dist, ent->mover.movedir, ent->mover.end_origin );

	VectorClear( ent->s.ms.angles );
	VectorCopy( ent->s.ms.angles, ent->mover.start_angles );
	VectorCopy( ent->s.ms.angles, ent->mover.end_angles );

	if( ent->spawnflags & MOVER_FLAG_REVERSE )
	{
		VectorCopy( ent->mover.end_origin, ent->s.ms.origin );
		VectorCopy( ent->mover.start_origin, ent->mover.end_origin );
		VectorCopy( ent->s.ms.origin, ent->mover.start_origin );
		VectorNegate( ent->mover.movedir, ent->mover.movedir );
	}

	ent->mover.state = MOVER_STATE_STARTPOS;
	GClip_LinkEntity( ent );
}

//==================================================
// FUNC ROTATING
//==================================================

/*
* G_Rotating_Touch
*/
static void G_Rotating_Touch( gentity_t *ent, gentity_t *other, cplane_t *plane, int surfFlags )
{
	//if( ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2] )
	//	G_TakeDamage( other, ent, ent, vec3_origin, ent->dmg, 1, 0, MOD_CRUSH );
}

/*
* G_Func_Rotating
* func_rotating starts on by default. If used stops. When the reverse flag is
* enabled, it starts off and if used starts. The toggle flag allows to stop and start forever
*/
void G_Func_Rotating( gentity_t *ent )
{
	G_InitMover( ent );

	ent->mover.speed = st.speed;
	if( ent->mover.speed <= 0 )
	{
		ent->mover.speed = 400;
	}

	ent->mover.accel = st.accel;
	if( ent->mover.accel <= 0 )
	{
		ent->mover.accel = 40;
	}

	ent->mover.wait = ent->wait * 1000;

	ent->mover.sound = G_LocalSound( SOUND_ROTATING_MOVE );

	ent->activate = G_Rotating_Activate;
	ent->blocked = G_Rotating_Blocked;
	ent->touch = G_Rotating_Touch;
	ent->die = G_Mover_Die;
	ent->pain = NULL;

	G_RotationFromAngles( ent->s.ms.angles, ent->mover.movedir );

	VectorClear( ent->s.ms.angles );
	VectorClear( ent->s.ms.velocity );

	// the start and end pos angles in this case mean the velocities
	if( ent->spawnflags & MOVER_FLAG_REVERSE )
	{
		VectorClear( ent->mover.start_angles );
		VectorScale( ent->mover.movedir, ent->mover.speed, ent->mover.end_angles );
		VectorClear( ent->avelocity );
		ent->s.ms.type = MOVE_TYPE_NONE;
		ent->think = NULL;
	}
	else
	{
		VectorClear( ent->mover.end_angles );
		VectorScale( ent->mover.movedir, ent->mover.speed, ent->mover.start_angles );
		VectorCopy( ent->mover.start_angles, ent->avelocity );
		ent->s.ms.type = MOVE_TYPE_PUSHER;
		ent->s.sound = ent->mover.sound;
		ent->think = NULL;
	}

	ent->mover.state = MOVER_STATE_STARTPOS;

	GClip_LinkEntity( ent );
}

/*
* G_Func_BrushModel
*/
void G_Func_BrushModel( gentity_t *ent )
{
	GClip_SetBrushModel( ent, ent->model );

	ent->s.solid = SOLID_SOLID;
	ent->netflags &= ~SVF_NOCLIENT;

	ent->s.ms.type = MOVE_TYPE_NONE;

	ent->die = G_Mover_Die;

	GClip_LinkEntity( ent );
}
