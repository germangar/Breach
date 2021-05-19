/*
   Copyright (C) 2007 German Garcia
 */

#include "gs_local.h"

//#define CHECK_TRAPPED

//==================================================
// SLIDE MOVE
//
// Note: environment info should be up to date when calling any slide move function
//==================================================

#define SLIDEMOVE_PLANEINTERACT_EPSILON	0.05
#define SLIDEMOVEFLAG_PLANE_TOUCHED 16
#define SLIDEMOVEFLAG_WALL_BLOCKED  8
#define SLIDEMOVEFLAG_TRAPPED	    4
#define SLIDEMOVEFLAG_BLOCKED	    2   // it was blocked at some point, doesn't mean it didn't slide along the blocking object
#define SLIDEMOVEFLAG_MOVED	    1

//=================
//GS_ClearClippingPlanes
//=================
static void GS_ClearClippingPlanes( move_t *move )
{
	move->local.numClipPlanes = 0;
}

//=================
//GS_ClipVelocityToClippingPlanes
//=================
static void GS_ClipVelocityToClippingPlanes( move_t *move )
{
	int i;

	for( i = 0; i < move->local.numClipPlanes; i++ )
	{
		if( DotProduct( move->ms->velocity, move->local.clipPlaneNormals[i] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
			continue; // looking in the same direction than the velocity

#ifndef TRACEVICFIX
#ifndef TRACE_NOAXIAL
		// this is a hack, cause non axial planes can return invalid positions in trace endpos
		if( PlaneTypeForNormal( move->local.clipPlaneNormals[i] ) == PLANE_NONAXIAL )
		{
			vec3_t vec;
			int j;
			// offset the origin a little bit along the plane normal
			VectorMA( move->ms->origin, 0.1, move->local.clipPlaneNormals[i], vec );
			VectorSubtract( vec, move->ms->origin, vec ); // vec is the distance to move
			// clip this offset to the other clipping planes
			for( j = 0; j < move->local.numClipPlanes; j++ )
			{
				if( i == j || DotProduct( vec, move->local.clipPlaneNormals[j] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
					continue; // looking in the same direction than the vec
				GS_ClipVelocity( vec, move->local.clipPlaneNormals[j], vec );
			}
			VectorAdd( move->ms->origin, vec, move->ms->origin );
		}
#endif
#endif

		if( move->bounceFrac > 1.0f )
			GS_BounceVelocity( move->ms->velocity, move->local.clipPlaneNormals[i], move->ms->velocity, move->bounceFrac );
		else
			GS_ClipVelocity( move->ms->velocity, move->local.clipPlaneNormals[i], move->ms->velocity );
	}
}

//=================
//GS_AddClippingPlane
//=================
static void GS_AddClippingPlane( move_t *move, const vec3_t planeNormal )
{
	int i;

	// see if we are already clipping to this plane
	for( i = 0; i < move->local.numClipPlanes; i++ )
	{
		if( DotProduct( planeNormal, move->local.clipPlaneNormals[i] ) >= ( 1.0f - SLIDEMOVE_PLANEINTERACT_EPSILON ) )
		{
			return;
		}
	}

	if( move->local.numClipPlanes + 1 == MAX_SLIDEMOVE_CLIP_PLANES )
		GS_Error( "GS_AddTouchPlane: MAX_SLIDEMOVE_CLIP_PLANES reached\n" );

	// add the plane
	VectorCopy( planeNormal, move->local.clipPlaneNormals[move->local.numClipPlanes] );
	move->local.numClipPlanes++;
}

//=================
//GS_StepUp
//=================
static qboolean GS_StepUp( move_t *move )
{
	vec3_t backup_origin, end, dir;
	trace_t	trace;

	//if( move->env.groundentity == ENTITY_INVALID )
	//	return qfalse;

	VectorCopy( move->ms->origin, backup_origin );

	// move up by the size of a step
	VectorMA( move->ms->origin, -STEPSIZE, gs.environment.gravityDir, move->ms->origin );
	VectorNormalize2( move->ms->velocity, dir );

	// move forward for step checking
	VectorMA( move->ms->origin, 0.25, dir, end );
	GS_Trace( &trace, move->ms->origin, move->local.mins, move->local.maxs, end, move->entNum, move->contentmask, 0 );
	if( trace.startsolid || trace.allsolid || trace.fraction < 1.0f )
	{                                                               // found a wall
		VectorCopy( backup_origin, move->ms->origin );
		return qfalse;
	}

	// didn't find obstacle, so put it at ground again
	VectorCopy( trace.endpos, move->ms->origin );
	VectorMA( move->ms->origin, STEPSIZE, gs.environment.gravityDir, end );
	GS_Trace( &trace, move->ms->origin, move->local.mins, move->local.maxs, end, move->entNum, move->contentmask, 0 );
	if( trace.startsolid || trace.allsolid || trace.fraction == 1.0f ) // no ground found
	{
		VectorCopy( backup_origin, move->ms->origin );
		return qfalse;
	}
	// found ground, but it is not walkable, so not a valid step
	if( !IsGroundPlane( trace.plane.normal, gs.environment.gravityDir ) )
	{
		VectorCopy( backup_origin, move->ms->origin );
		return qfalse;
	}

	move->output.step += trace.endpos[2] - backup_origin[2]; // store the heigh for view smoothing
	VectorCopy( trace.endpos, move->ms->origin );
	move->ms->velocity[2] = 0; // if stepped succesfully he's on ground so he doesn't have vertical velocity anymore
	return qtrue;
}

//=================
//GS_SlideMoveClipMove
//=================
static int GS_SlideMoveClipMove( move_t *move, const qboolean stepping )
{
	vec3_t endpos, startingOrigin, startingVelocity;
	trace_t	trace;
	int blockedmask = 0;

	VectorCopy( move->ms->origin, startingOrigin );
	VectorCopy( move->ms->velocity, startingVelocity );

	VectorMA( move->ms->origin, move->local.remainingTime, move->ms->velocity, endpos );
	GS_Trace( &trace, move->ms->origin, move->local.mins, move->local.maxs, endpos, move->entNum, move->contentmask, 0 );
	if( trace.allsolid )
	{
		return blockedmask|SLIDEMOVEFLAG_TRAPPED;
	}

	if( trace.fraction == 1.0f )
	{                          // was able to cleanly perform the full move
		VectorCopy( trace.endpos, move->ms->origin );
		move->local.remainingTime -= ( trace.fraction * move->local.remainingTime );
		return blockedmask|SLIDEMOVEFLAG_MOVED;
	}

	if( trace.fraction < 1.0f )
	{                         // wasn't able to make the full move
		GS_AddTouchEnt( &move->output.touchList, trace.ent, &trace.plane, trace.surfFlags );
		blockedmask |= SLIDEMOVEFLAG_PLANE_TOUCHED;

		// move what can be moved
		if( trace.fraction > 0.0 )
		{
			VectorCopy( trace.endpos, move->ms->origin );
			move->local.remainingTime -= ( trace.fraction * move->local.remainingTime );
			blockedmask |= SLIDEMOVEFLAG_MOVED;
		}

		// if the plane is a wall and stepping, try to step it up
		if( !IsGroundPlane( trace.plane.normal, gs.environment.gravityDir ) )
		{
			if( stepping && GS_StepUp( move ) )
			{
				return blockedmask; // solved : don't add the clipping plane
			}
			else
			{
				blockedmask |= SLIDEMOVEFLAG_WALL_BLOCKED;
			}
		}

		GS_AddClippingPlane( move, trace.plane.normal );
	}

	return blockedmask;
}

//=================
//GS_SlideMove
//=================
static int GS_SlideMove( move_t *move, float time, const qboolean stepping )
{
#define MAX_SLIDEMOVE_ATTEMPTS	8
	int count;
	int blockedmask = 0;
	vec3_t lastValidOrigin, originalVelocity;

	// if the velocity is too small, just stop
	if( VectorLength( move->ms->velocity ) < 1 )
	{
		VectorClear( move->ms->velocity );
		move->local.remainingTime = 0;
		return 0;
	}

	VectorCopy( move->ms->velocity, originalVelocity );
	VectorCopy( move->ms->origin, lastValidOrigin );

	move->local.remainingTime = time;

	GS_ClearClippingPlanes( move );

	for( count = 0; count < MAX_SLIDEMOVE_ATTEMPTS; count++ )
	{
		// get the original velocity and clip it to all the planes we got in the list
		VectorCopy( originalVelocity, move->ms->velocity );
		GS_ClipVelocityToClippingPlanes( move );
		blockedmask = GS_SlideMoveClipMove( move, stepping );

#ifdef CHECK_TRAPPED
		if( gs.module == GS_MODULE_GAME )
		{
			trace_t	trace;
			GS_Trace( &trace, move->ms->origin, move->local.mins, move->local.maxs, move->ms.origin, move->entNum, move->contentmask, 0 );
			if( trace.startsolid )
			{
				blockedmask |= SLIDEMOVEFLAG_TRAPPED;
			}
		}
#endif

		// can't continue
		if( blockedmask & SLIDEMOVEFLAG_TRAPPED )
		{
#ifdef CHECK_TRAPPED
			if( gs.module == GS_MODULE_GAME )
				GS_Printf( "GS_SlideMove SLIDEMOVEFLAG_TRAPPED\n" );
#endif
			move->local.remainingTime = 0.0f;
			VectorCopy( lastValidOrigin, move->ms->origin );
			return blockedmask;
		}

		VectorCopy( move->ms->origin, lastValidOrigin );

		// touched a plane, re-clip velocity and retry
		if( blockedmask & SLIDEMOVEFLAG_PLANE_TOUCHED )
		{
			continue;
		}

		// if it didn't touch anything the move should be completed
		if( move->local.remainingTime > 0.0f )
		{
			GS_Printf( "GS_SlideMove finished with remaining time\n" );
			move->local.remainingTime = 0.0f;
		}

		break;
	}

	return blockedmask;
}

//=================
//GS_StepSlideMove
// handles player stair stepping
//=================
void GS_BoxStepSlideMove( move_t *move )
{
	vec3_t end;
	trace_t	trace;
	qboolean step_down;

	step_down = (qboolean)( move->env.groundentity != ENTITY_INVALID );

	GS_SlideMove( move, move->local.frameTime, qtrue );

	// if it originally had ground, try to step down
	if( step_down && !move->output.step && move->ms->velocity[2] <= 0 )
	{

		// see if it lost ground
		VectorMA( move->ms->origin, 0.25, gs.environment.gravityDir, end );
		GS_Trace( &trace, move->ms->origin, move->local.mins, move->local.maxs, end, move->entNum, move->contentmask, 0 );
		if( ( trace.fraction == 1 ) || ( !IsGroundPlane( trace.plane.normal, gs.environment.gravityDir ) && !trace.startsolid ) )
		{
			// no ground found. try to step down
			VectorMA( move->ms->origin, STEPSIZE, gs.environment.gravityDir, end );
			GS_Trace( &trace, move->ms->origin, move->local.mins, move->local.maxs, end, move->entNum, move->contentmask, 0 );
			if( !trace.startsolid && !trace.allsolid && trace.fraction < 1.0f )
			{

#ifndef TRACEVICFIX
#ifndef TRACE_NOAXIAL
				// this is a hack, cause non axial planes can return invalid positions in trace endpos
				if( PlaneTypeForNormal( trace.plane.normal ) == PLANE_NONAXIAL )
				{
					int j;
					vec3_t vec;
#define LESSSLIDEONSLOPE
#ifdef LESSSLIDEONSLOPE
					// offset the origin a little bit along the plane normal
					VectorMA( trace.endpos, 0.25, trace.plane.normal, vec );
#else
					// offset the origin a little bit along the plane normal
					// It was 0.1 but made it slide sideways on terrain slopes
					VectorMA( trace.endpos, 0.025, trace.plane.normal, vec );
#endif

					VectorSubtract( vec, trace.endpos, vec ); // vec is the distance to move
					// clip this offset to the other clipping planes
					for( j = 0; j < move->local.numClipPlanes; j++ )
					{
						if( DotProduct( vec, move->local.clipPlaneNormals[j] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
							continue; // looking in the same direction than the vec
						GS_ClipVelocity( vec, move->local.clipPlaneNormals[j], vec );
					}
					VectorAdd( trace.endpos, vec, trace.endpos );
				}
#endif
#endif
				move->output.step += trace.endpos[2] - move->ms->origin[2]; // store the heigh for view smoothing
				VectorCopy( trace.endpos, move->ms->origin );
				// if stepped succesfully he's on ground so he doesn't have vertical velocity anymore
				move->ms->velocity[2] = 0;
			}
		}
	}
}

//=================
//GS_FlySlideMove
// no player stair stepping
//=================
void GS_BoxSlideMove( move_t *move )
{
	GS_SlideMove( move, move->local.frameTime, qfalse );
}

//=================
//GS_LinearMove
//=================
int GS_BoxLinearMove( move_t *move )
{
	int blockedmask = 0;
	vec3_t lastValidOrigin;

	// if the velocity is too small, just stop
	if( VectorLength( move->ms->velocity ) < 1 )
	{
		VectorClear( move->ms->velocity );
		move->local.remainingTime = 0;
		return 0;
	}

	VectorCopy( move->ms->origin, lastValidOrigin );

	move->local.remainingTime = move->local.frameTime;

	// do the move
	blockedmask = GS_SlideMoveClipMove( move, qfalse );

	// something went wrong. Revert to a sane position, but keep the impact info
	if( blockedmask & SLIDEMOVEFLAG_TRAPPED )
	{
		VectorCopy( lastValidOrigin, move->ms->origin );
		move->local.remainingTime = 0.0f;
		return blockedmask;
	}

	// sanely touched a plane, we're done
	if( blockedmask & SLIDEMOVEFLAG_PLANE_TOUCHED )
	{
		move->local.remainingTime = 0.0f;
	}

	// if it didn't touch anything the move should be completed
	if( move->local.remainingTime > 0.0f )
	{
		GS_Printf( "GS_LinearMove finished with remaining time\n" );
		move->local.remainingTime = 0.0f;
	}

	return blockedmask;
}
