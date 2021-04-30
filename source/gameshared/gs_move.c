/*
Copyright (C) 2007 German Garcia
*/

#include "gs_local.h"

/*
* GS_Move_EnvironmentForBox
*/
void GS_Move_EnvironmentForBox( moveenvironment_t *env, vec3_t origin, vec3_t velocity, vec3_t mins, vec3_t maxs, int passent, int contentmask, const movespecificts_t *customSpecifics )
{
	vec3_t point;
	trace_t	trace;
	const movespecificts_t *specifics;

	if( !( contentmask & MASK_SOLID ) )
	{
		env->groundentity = ENTITY_INVALID;
		env->waterlevel = env->watertype = 0;
	}
	else
	{
		env->waterlevel = GS_WaterLevelForBBox( origin, mins, maxs, &env->watertype );
		if( env->waterlevel & WATERLEVEL_FLOAT )
		{
			env->groundentity = ENTITY_INVALID;
		}
		else //  find ground
		{
			VectorMA( origin, 0.25, gs.environment.gravityDir, point );

			GS_Trace( &trace, origin, mins, maxs, point, passent, contentmask, 0 );
			env->groundplane = trace.plane;
			env->groundsurfFlags = trace.surfFlags;
			env->groundcontents = trace.contents;

			if( ( trace.fraction == 1 ) || ( !IsGroundPlane( trace.plane.normal, gs.environment.gravityDir ) && !trace.startsolid ) )
			{
				env->groundentity = ENTITY_INVALID;
			}
			else
			{
				env->groundentity = trace.ent;
			}
		}
	}

	specifics = customSpecifics ? customSpecifics : &defaultObjectMoveSpecs;

	// if it's not being clipped (freefly cam) we always apply "ground" values
	if( !contentmask )
		env->frictionFrac = specifics->frictionGround; // ghost movement
	else
	{
		if( env->waterlevel & WATERLEVEL_FLOAT )
			env->frictionFrac = specifics->frictionWater;
		else if( env->groundentity == ENTITY_INVALID )
			env->frictionFrac = specifics->frictionAir;
		else
		{
			env->frictionFrac = specifics->frictionGround;
			if( env->groundsurfFlags & SURF_SLICK )
				env->frictionFrac *= 0.1f;
		}
	}
}

/*
* GS_Move_AddAccelToVelocity
*/
static void GS_Move_AddAccelToVelocity( vec3_t accel, vec3_t velocity )
{
	float sv_maxSpeed = 0, speed;
	vec3_t clampVel;

	VectorAdd( velocity, accel, velocity );
	if( sv_maxSpeed > 0.0f )
	{
		speed = VectorNormalize2( velocity, clampVel );
		if( speed > sv_maxSpeed )
			VectorScale( clampVel, sv_maxSpeed, velocity );
	}
}

/*
* GS_Move_ApplyFrictionToVector
*/
void GS_Move_ApplyFrictionToVector( vec3_t vector, const vec3_t velocity, const float friction, const float frametime, const qboolean freefly )
{
	vec3_t frictionVec, curVelocity;
	float speed, fspeed;

	if( friction <= 0.0f )
		return;

	VectorCopy( velocity, curVelocity );

	if( freefly )
	{
		speed = VectorNormalize2( curVelocity, frictionVec );
		if( speed )
		{
			fspeed = friction * frametime;
			if( fspeed > speed )
				fspeed = speed;

			VectorMA( vector, -fspeed, frictionVec, vector );
		}
	}
	else
	{
		// on gravity movement, friction is applied to 2 different vectors, the horizontal and vertical
		vec3_t v;

		// hvel
		VectorSet( v, curVelocity[0], curVelocity[1], 0 );
		speed = VectorNormalize2( v, frictionVec );
		if( speed ) 
		{
			fspeed = friction * frametime;
			if( fspeed > speed )
				fspeed = speed;

			VectorMA( vector, -fspeed, frictionVec, vector );
		}

		// vvel
		VectorSet( v, 0, 0, curVelocity[2] );
		speed = VectorNormalize2( v, frictionVec );
		if( speed ) 
		{
			fspeed = friction * frametime;
			if( fspeed > speed )
				fspeed = speed;

			VectorMA( vector, -fspeed, frictionVec, vector );
		}
	}
}

/*
* GS_Move_AddAccelFromGravity
*/
static void GS_Move_AddAccelFromGravity( moveenvironment_t *env, vec3_t newaccel, float frametime )
{
	float gravityFactor;

	gravityFactor = gs.environment.gravity;
	if( env->waterlevel & WATERLEVEL_FLOAT )
	{
		gravityFactor *= 0.6f;
	}

	if( env->groundentity == ENTITY_INVALID )
	{
		VectorMA( newaccel, gravityFactor * frametime, gs.environment.gravityDir, newaccel );
	}
}

/*
* GS_WontMove
*/
static qboolean GS_WontMove( move_t *move )
{
	if( move->ms->type == MOVE_TYPE_LINEAR_PROJECTILE )
		return qfalse; // always moves

	if( VectorCompare( move->ms->velocity, vec3_origin ) && VectorCompare( move->accel, vec3_origin ) )
	{
		// only gravity could make them move
		if( move->ms->type == MOVE_TYPE_STEP || move->ms->type == MOVE_TYPE_OBJECT )
		{
			return (qboolean)( move->env.groundentity != ENTITY_INVALID );
		}

		return qtrue; // nothing will make them move
	}

	return qfalse;
}

/*
* GS_Move
*/
void GS_Move( move_t *move, unsigned int msecs, vec3_t mins, vec3_t maxs )
{
	int oldwaterlevel, oldgroundentity;
	float oldfallvelocity;

	// output is always cleared
	memset( &move->output, 0, sizeof( moveoutput_t ) );

	// set up local move data
	memset( &move->local, 0, sizeof( movelocal_t ) );
	move->local.frameTime = move->local.remainingTime = (float)msecs * 0.001f; // convert time to seconds
	VectorCopy( mins, move->local.mins );
	VectorCopy( maxs, move->local.maxs );

	if( !move->specifics )
		move->specifics = &defaultObjectMoveSpecs;

	GS_Move_EnvironmentForBox( &move->env, move->ms->origin, move->ms->velocity, move->local.mins, move->local.maxs, move->entNum, move->contentmask, move->specifics );

	if( msecs <= 0 )
		return;

	if( GS_WontMove( move ) ) // avoid executing the code if there are no chances of being moved
		return;

	// keep some "before" values to add events comparing them with "after" values
	oldwaterlevel = move->env.waterlevel;
	oldgroundentity = move->env.groundentity;
	oldfallvelocity = move->ms->velocity[2];

	switch( move->ms->type )
	{
	case MOVE_TYPE_NONE:
		break;
	case MOVE_TYPE_STEP:
		if( move->env.frictionFrac > 0.0f )
			GS_Move_ApplyFrictionToVector( move->ms->velocity, move->ms->velocity, move->env.frictionFrac, move->local.frameTime, qfalse );

		if( move->contentmask )
			GS_Move_AddAccelFromGravity( &move->env, move->accel, move->local.frameTime );

		GS_Move_AddAccelToVelocity( move->accel, move->ms->velocity );
		GS_BoxStepSlideMove( move );
		break;

	case MOVE_TYPE_FREEFLY:
		if( move->env.frictionFrac > 0.0f )
			GS_Move_ApplyFrictionToVector( move->ms->velocity, move->ms->velocity, move->env.frictionFrac, move->local.frameTime, qtrue );

		if( move->contentmask )
			GS_Move_AddAccelFromGravity( &move->env, move->accel, move->local.frameTime );

		GS_Move_AddAccelToVelocity( move->accel, move->ms->velocity );
		GS_BoxStepSlideMove( move );
		break;

	case MOVE_TYPE_OBJECT:
		if( move->env.frictionFrac > 0.0f )
			GS_Move_ApplyFrictionToVector( move->ms->velocity, move->ms->velocity, move->env.frictionFrac, move->local.frameTime, qfalse );

		if( move->contentmask )
			GS_Move_AddAccelFromGravity( &move->env, move->accel, move->local.frameTime );

		GS_Move_AddAccelToVelocity( move->accel, move->ms->velocity );
		GS_BoxSlideMove( move );
		break;

	case MOVE_TYPE_LINEAR:
		GS_BoxLinearMove( move );
		break;

	case MOVE_TYPE_PUSHER:
		break;

	case MOVE_TYPE_LINEAR_PROJECTILE:
		break;

	default:
		GS_Error( "GS_Move: Invalid movetype\n" );
		break;
	}

	// update groundentity & waterlevel to the new position
	GS_Move_EnvironmentForBox( &move->env, move->ms->origin, move->ms->velocity, move->local.mins, move->local.maxs, move->entNum, move->contentmask, move->specifics );

	// add event effects
	if( ( oldfallvelocity < -1.0f ) && ( oldgroundentity == ENTITY_INVALID ) && ( move->env.groundentity != ENTITY_INVALID ) )
	{
		int delta = ( -oldfallvelocity * 0.1f );
		if( delta > 50 && !( move->env.waterlevel & WATERLEVEL_FLOAT ) )
		{
			delta -= 50;
			clamp( delta, 0, 255 );
			module.predictedEvent( move->entNum, EV_FALLIMPACT, delta );
		}
	}

	// if entered water, kill out velocity
	if( !oldwaterlevel && ( move->env.waterlevel & WATERLEVEL_FLOAT ) )
	{
		move->ms->velocity[0] *= 0.50f;
		move->ms->velocity[1] *= 0.50f;
		move->ms->velocity[2] *= 0.25f;
		module.predictedEvent( move->entNum, EV_WATERSPLASH, 0 );
	}
	else if( !( oldwaterlevel & WATERLEVEL_FEETS ) && ( move->env.waterlevel & WATERLEVEL_FEETS ) )
	{
		module.predictedEvent( move->entNum, EV_WATERENTER, 0 );
	}
	else if( oldwaterlevel && !( move->env.waterlevel & WATERLEVEL_FEETS ) )
	{
		module.predictedEvent( move->entNum, EV_WATEREXIT, 0 );
	}

	// snap
	GS_SnapVector( move->ms->velocity );
#ifdef MOVE_SNAP_ORIGIN
	GS_SnapPosition( move->ms->origin, move->local.mins, move->local.maxs, move->entNum, move->contentmask );
#endif
}

/*
* GS_Move_LinearProjectile - is a special case of movement which doesn't use GS_Move
*/
touchlist_t *GS_Move_LinearProjectile( entity_state_t *state, unsigned int curtime, vec3_t neworigin, int passent, int clipmask, int timeDelta )
{
	static touchlist_t touchList;
	vec3_t end;
	int mask = MASK_SHOT;
	trace_t	trace;
	float flyTime;

	touchList.numtouch = 0;

	if( curtime > state->ms.linearProjectileTimeStamp )
		flyTime = (float)( curtime - state->ms.linearProjectileTimeStamp ) * 0.001f;
	else
		flyTime = 0.0f;

	VectorMA( state->origin2, flyTime, state->ms.velocity, end );
	if( state->skinindex )
	{
		vec3_t dir;
		VectorScale( gs.environment.gravityDir, state->skinindex * 8.0f, dir );
		VectorMA( end, ( flyTime*flyTime )*0.5, dir, end );
	}

	if( !clipmask ) // don't collide
		VectorCopy( end, neworigin );
	else
	{
		GS_Trace( &trace, state->ms.origin, state->local.boundmins, state->local.boundmaxs, end, passent, mask, timeDelta );
		VectorCopy( trace.endpos, neworigin );

		if( trace.ent != ENTITY_INVALID )
			GS_AddTouchEnt( &touchList, trace.ent, &trace.plane, trace.surfFlags );
	}

	return &touchList;
}

