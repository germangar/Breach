/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

//==================================================
//
// BINARY MOVERS
//
//==================================================

#define DEFAULT_MOVER_HEALTH	800

/*
* G_InitMover
*/
void G_InitMover( gentity_t *ent )
{
	memset( &ent->mover, 0, sizeof( g_moverinfo_t ) );
	ent->s.solid = SOLID_SOLID;
	GClip_SetBrushModel( ent, ent->model );
	ent->s.ms.type = MOVE_TYPE_PUSHER;
	ent->netflags &= ~SVF_NOCLIENT;

	if( ent->model2 )
	{
		ent->s.modelindex2 = trap_ModelIndex( ent->model2 );
		trap_PureModel( ent->model2 );
	}

	if( ent->wait < 0 )
		ent->wait = 0;
	if( ent->health < 0 )
		ent->health = 0;
	if( ent->damage < 0 )
		ent->damage = 0;

	// TEMP FIXME: allow every mover to be activated by projectiles by now
	ent->spawnflags |= MOVER_FLAG_ALLOW_SHOT_ACTIVATION;

	if( ent->spawnflags & MOVER_FLAG_TAKEDAMAGE )
		ent->s.flags |= SFL_TAKEDAMAGE;

	if( ent->health < 1 && ( ent->s.flags & SFL_TAKEDAMAGE ) )
		ent->health = DEFAULT_MOVER_HEALTH;
}

/*
* G_Mover_Die
*/
void G_Mover_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage )
{
	GS_Printf( "G_Mover_Die: Damage %i Health: %.1f\n", damage, ent->health );
	G_FreeEntity( ent );
}

//==================================================
// MOVERS THINKING ROUTINES
//==================================================

/*
* G_Mover_Done
*/
static void G_Mover_Done( gentity_t *ent )
{
	VectorCopy( ent->mover.dest, ent->s.ms.origin );
	VectorCopy( ent->mover.destangles, ent->s.ms.angles );
	VectorClear( ent->s.ms.velocity );
	VectorClear( ent->avelocity );
	ent->mover.endfunc( ent );
}

/*
* G_Mover_Watch
*/
static void G_Mover_Watch( gentity_t *ent )
{
	vec3_t dir;
	float dist;
	qboolean reached, areached;
	float secsFrameTime;

	reached = areached = qfalse;

	secsFrameTime = (float)game.framemsecs * 0.001f;

	if( VectorCompare( ent->mover.dest, ent->s.ms.origin ) )
	{
		VectorClear( ent->s.ms.velocity );
		reached = qtrue;
	}
	else
	{  // see if it's going to be reached, and adjust
		float speed;

		VectorSubtract( ent->mover.dest, ent->s.ms.origin, dir );
		dist = VectorNormalize( dir );
		speed = ent->mover.speed;
		if( dist < ( speed * secsFrameTime ) )
		{
			speed = dist / secsFrameTime;
		}
		VectorScale( dir, speed, ent->s.ms.velocity );
	}

	if( VectorCompare( ent->mover.destangles, ent->s.ms.angles ) )
	{
		VectorClear( ent->avelocity );
		areached = qtrue;
	}
	else
	{  // see if it's going to be reached, and adjust
		float speed;

		VectorSubtract( ent->mover.destangles, ent->s.ms.angles, dir );
		dist = VectorNormalize( dir );
		speed = VectorLength( ent->avelocity );
		if( dist < ( speed * secsFrameTime ) )
		{
			speed = dist / secsFrameTime;
		}
		VectorScale( dir, speed, ent->avelocity );
	}

	if( reached && areached )
	{
		ent->think =  G_Mover_Done;
		ent->nextthink = level.time + 1;
		return;
	}
	ent->think =  G_Mover_Watch;
	ent->nextthink = level.time + 1;
}

/*
* G_Mover_Begin
*/
static void G_Mover_Begin( gentity_t *ent )
{
	vec3_t dir, destdelta;
	float remainingdist, angledist, anglespeed, time;

	// set up velocity vector
	VectorSubtract( ent->mover.dest, ent->s.ms.origin, dir );
	remainingdist = VectorNormalize( dir );
	if( !remainingdist )
	{
		time = 0;
		VectorClear( ent->s.ms.velocity );
	}
	else
	{
		time = remainingdist / ent->mover.speed;
		VectorScale( dir, ent->mover.speed, ent->s.ms.velocity );
		GS_SnapVector( ent->s.ms.velocity );
	}

	// set up avelocity vector
	VectorSubtract( ent->mover.destangles, ent->s.ms.angles, destdelta );
	angledist = VectorNormalize( destdelta );
	if( time )
	{        // if time was set up by traveling distance, scale angle speed to fit it
		anglespeed = angledist / time;
	}
	else
	{
		anglespeed = ent->mover.speed;
	}
	VectorScale( destdelta, anglespeed, ent->avelocity );

	G_Mover_Watch( ent );
}

/*
* G_Mover_Calc
*/
static void G_Mover_Calc( gentity_t *ent, vec3_t dest_origin, vec3_t dest_angles, void ( *func )(gentity_t *) )
{
	ent->mover.endfunc = func;
	VectorCopy( dest_origin, ent->mover.dest );
	GS_SnapVector( ent->mover.dest ); // it's important to snap the destiny, or might never be reached.
	VectorCopy( dest_angles, ent->mover.destangles );
	VectorClear( ent->s.ms.velocity );
	VectorClear( ent->avelocity );

	G_Mover_Begin( ent );
}

static void G_Mover_Go_Start( gentity_t *ent );

/*
* G_Mover_Hit_End
*/
static void G_Mover_Hit_End( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_endpos, 0, qtrue );
	ent->s.sound = 0;
	ent->mover.state = MOVER_STATE_ENDPOS;
	ent->nextthink = level.time + ent->mover.wait;
	if( ent->spawnflags & MOVER_FLAG_TOGGLE )
		ent->think = NULL;
	else
		ent->think = G_Mover_Go_Start;

	G_ActivateTargets( ent, ent->mover.activator );

	if( ent->mover.is_areaportal && ( ent->spawnflags & MOVER_FLAG_REVERSE ) )
		G_Door_SetAreaportalState( ent, qfalse );
}

/*
* G_Mover_Hit_Start
*/
static void G_Mover_Hit_Start( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_startpos, 0, qtrue );
	ent->s.sound = 0;

	ent->think = NULL;
	ent->nextthink = level.time + ent->mover.wait;
	ent->mover.activator = NULL;
	ent->mover.state = MOVER_STATE_STARTPOS;

	if( ent->mover.is_areaportal && !( ent->spawnflags & MOVER_FLAG_REVERSE ) )
		G_Door_SetAreaportalState( ent, qfalse );
}

/*
* G_Mover_Go_Start
*/
static void G_Mover_Go_Start( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_startreturning, 0, qtrue );
	ent->s.sound = ent->mover.sound;

	ent->mover.state = MOVER_STATE_GO_START;
	G_Mover_Calc( ent, ent->mover.start_origin, ent->mover.start_angles, G_Mover_Hit_Start );

	if( ent->mover.is_areaportal && ( ent->spawnflags & MOVER_FLAG_REVERSE ) )
		G_Door_SetAreaportalState( ent, qtrue );
}

/*
* G_Mover_Go_End
*/
static void G_Mover_Go_End( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_startmoving, 0, qtrue );
	ent->s.sound = ent->mover.sound;

	ent->mover.state = MOVER_STATE_GO_END;
	G_Mover_Calc( ent, ent->mover.end_origin, ent->mover.end_angles, G_Mover_Hit_End );

	if( ent->mover.is_areaportal && !( ent->spawnflags & MOVER_FLAG_REVERSE ) )
		G_Door_SetAreaportalState( ent, qtrue );
}

/*
* G_Mover_Blocked
*/
void G_Mover_Blocked( gentity_t *ent, gentity_t *other )
{
	if( other && ( ent->spawnflags & MOVER_FLAG_CRUSHER || !other->client ) )
	{
		G_TakeDamage( other, ent, ent, vec3_origin, 100000, 1, DAMAGE_NO_PROTECTION, MOD_CRUSH );
		if( other && !other->client && ( other->s.solid != SOLID_NOT ) )  // free non-clients if they are still blocking
			G_FreeEntity( other );
	}
	else
	{
		if( ent->damage )
			G_TakeDamage( other, ent, ent, vec3_origin, ent->damage, 1, 0, MOD_CRUSH );
		if( ent->mover.state == MOVER_STATE_GO_END )
			G_Mover_Go_Start( ent );
		else if( ent->mover.state == MOVER_STATE_GO_START )
			G_Mover_Go_End( ent );
	}
}

/*
* G_Mover_Activate
*/
void G_Mover_Activate( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if( ent->s.ms.velocity[0] || ent->s.ms.velocity[1] || ent->s.ms.velocity[2]
	|| ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2] )
		return; // already moving

	if( activator && ent->s.team && ( activator->s.team != ent->s.team ) )
		return;

	if( ( ent->spawnflags & MOVER_FLAG_TOGGLE ) && ( ent->nextthink <= level.time ) )
	{
		if( ent->mover.state == MOVER_STATE_STARTPOS )
		{
			ent->mover.activator = activator;
			G_Mover_Go_End( ent );
		}
		else if( ent->mover.state == MOVER_STATE_ENDPOS )
		{
			ent->mover.activator = activator;
			G_Mover_Go_Start( ent );
		}
	}
	else
	{
		if( ent->mover.state == MOVER_STATE_STARTPOS )
		{
			ent->mover.activator = activator;
			G_Mover_Go_End( ent );
		}
		// if doors, keep them open while we are touching the trigger
		else if( ent->mover.is_areaportal && ( ent->mover.state == MOVER_STATE_ENDPOS ) )
		{
			ent->nextthink = level.time + ent->mover.wait;
		}
	}
}

//==================================================
// ROTATORS
//==================================================

/*
* G_Rotating_Done
*/
static void G_Rotating_Done( gentity_t *ent )
{
	ent->mover.endfunc( ent );
}

/*
* G_Rotating_Watch
*/
static void G_Rotating_Watch( gentity_t *ent )
{
	float secsFrameTime;
	float curspeed, destspeed;
	vec3_t adir;

	secsFrameTime = (float)game.framemsecs * 0.001f;

	destspeed = VectorLength( ent->mover.destangles );
	curspeed = VectorLength( ent->avelocity );

	if( destspeed )
		VectorNormalize2( ent->mover.destangles, adir );
	else
		VectorNormalize2( ent->avelocity, adir );

	if( curspeed < destspeed )
	{
		curspeed += ent->mover.accel * secsFrameTime;
		if( curspeed > destspeed )
			curspeed = destspeed;
	}
	else if( curspeed > destspeed )
	{
		curspeed -= ent->mover.accel * secsFrameTime;
		if( curspeed < destspeed )
			curspeed = destspeed;
	}

	VectorScale( adir, curspeed, ent->avelocity );

	if( VectorCompare( ent->avelocity, ent->mover.destangles ) )
	{
		ent->think = G_Rotating_Done;
		ent->nextthink = level.time + 1;
		return;
	}

	ent->think = G_Rotating_Watch;
	ent->nextthink = level.time + 1;
}

/*
* G_Rotating_Calc
*/
static void G_Rotating_Calc( gentity_t *ent, vec3_t dest_avelocity, void ( *func )(gentity_t *) )
{
	ent->s.ms.type = MOVE_TYPE_PUSHER;
	ent->mover.endfunc = func;
	VectorCopy( dest_avelocity, ent->mover.destangles );
	ent->think = G_Rotating_Watch;
	ent->nextthink = level.time + 1;
}

/*
* G_Rotating_Hit_End
*/
static void G_Rotating_Hit_End( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_endpos, 0, qtrue );
	ent->think = NULL;
	ent->mover.state = MOVER_STATE_ENDPOS;
	ent->mover.activator = NULL;

	G_ActivateTargets( ent, ent->mover.activator );

	if( ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2] )
	{
		ent->s.sound = ent->mover.sound;
	}
	else
	{
		ent->s.ms.type = MOVE_TYPE_NONE;
		ent->s.sound = 0;
	}
}

/*
* G_Rotating_Hit_Start
*/
static void G_Rotating_Hit_Start( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_startpos, 0, qtrue );
	ent->s.sound = 0;
	ent->think = NULL;
	ent->mover.state = MOVER_STATE_STARTPOS;
	ent->mover.activator = NULL;

	if( ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2] )
	{
		ent->s.sound = ent->mover.sound;
	}
	else
	{
		ent->s.ms.type = MOVE_TYPE_NONE;
		ent->s.sound = 0;
	}
}

/*
* G_Rotating_Go_End
*/
static void G_Rotating_Go_End( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_startmoving, 0, qtrue );
	ent->s.sound = ent->mover.sound;

	ent->mover.state = MOVER_STATE_GO_END;
	G_Rotating_Calc( ent, ent->mover.end_angles, G_Rotating_Hit_End );
}

/*
* G_Rotating_Go_Start
*/
static void G_Rotating_Go_Start( gentity_t *ent )
{
	G_AddEvent( ent, ent->mover.event_startmoving, 0, qtrue );
	ent->s.sound = ent->mover.sound;

	ent->mover.state = MOVER_STATE_GO_START;
	G_Rotating_Calc( ent, ent->mover.start_angles, G_Rotating_Hit_Start );
}

/*
* G_Rotating_Blocked
*/
void G_Rotating_Blocked( gentity_t *ent, gentity_t *other )
{
	if( ent->spawnflags & MOVER_FLAG_CRUSHER || ( other && !other->client ) )
	{
		G_TakeDamage( other, ent, ent, vec3_origin, 100000, 1, DAMAGE_NO_PROTECTION, MOD_CRUSH );
		if( other && !other->client && ( other->s.solid != SOLID_NOT ) )  // free non-clients if they are still blocking
			G_FreeEntity( other );
	}
	else
	{
		if( ent->damage )
			G_TakeDamage( other, ent, ent, vec3_origin, ent->damage, 1, 0, MOD_CRUSH );
	}
}

/*
* G_Rotating_Activate
*/
void G_Rotating_Activate( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if( ent->spawnflags & MOVER_FLAG_TOGGLE )
	{
		if( ent->mover.state == MOVER_STATE_STARTPOS )
		{
			ent->mover.activator = activator;
			G_Rotating_Go_End( ent );
		}
		else if( ent->mover.state == MOVER_STATE_ENDPOS )
		{                                           // it's on
			ent->mover.activator = activator;
			G_Rotating_Go_Start( ent );
		}
	}
	else
	{
		if( ent->mover.state == MOVER_STATE_STARTPOS )
		{
			ent->mover.activator = activator;
			G_Rotating_Go_End( ent );
		}
	}
}
