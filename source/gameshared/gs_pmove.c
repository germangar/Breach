/*
   Copyright (C) 1997-2001 Id Software, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 */

#include "gs_local.h"

//===============================================================
//		WARSOW player AAboxes sizes
/*
   vec3_t	playerbox_stand_mins = { -16, -16, -24 };
   vec3_t	playerbox_stand_maxs = { 16, 16, 40 };
   int		playerbox_stand_viewheight = 32;

   vec3_t	playerbox_crouch_mins = { -16, -16, -24 };
   vec3_t	playerbox_crouch_maxs = { 16, 16, 16 };
   int		playerbox_crouch_viewheight = 12;

   vec3_t	playerbox_gib_mins = { -16, -16, 0 };
   vec3_t	playerbox_gib_maxs = { 16, 16, 16 };
   int		playerbox_gib_viewheight = 8;
 */
/*
 #define PM_OVERBOUNCE			1.01f
 #define PM_SLIDE_CLAMPING
 #define NEW_PM_SLIDEMOVE

   //===============================================================

   // all of the locals will be zeroed before each
   // pmove, just to make damn sure we don't have
   // any differences when running on client or server

   typedef struct
   {
    vec3_t		origin;			// full float precision
    vec3_t		velocity;		// full float precision

    //vec3_t		forward, right, up;
    vec3_t		viewAxis[3];
    float		frametime;


    int			groundsurfFlags;
    cplane_t	groundplane;
    int			groundcontents;

    vec3_t		previous_origin;
    qboolean	ladder;

    float		fordwardPush, sidePush, upPush;
   } pml_t;

 #define SPEEDKEY	400

   pmove_t		*pm;
   pml_t		pml;


   // movement parameters
   float	pm_stopspeed = 100;
   float	pm_maxspeed = 320;
   float	pm_maxwalkspeed = 160;
   float	pm_duckspeed = 100;

   float	pm_accelerate = 15; // Kurim : initially 10
   float	pm_wateraccelerate = 10; // initially 6

   float	pm_friction = 8; // initially 6
   float	pm_waterfriction = 1;

   // wsw physics

   // cpm-like
   float	pm_airstopaccelerate = 2.5;
   float	pm_aircontrol = 150;
   float	pm_strafeaccelerate = 70;
   float	pm_wishspeed = 30;

   // jump
   float	pm_jump_velocity = 280;

   //
   // Kurim : some functions/defines that can be useful to work on the horizontal mouvement of player :
   //
 #define VectorScale2D(in,scale,out) ((out)[0]=(in)[0]*(scale),(out)[1]=(in)[1]*(scale))
 #define DotProduct2D(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1])

   static vec_t VectorNormalize2D( vec3_t v ) // ByMiK : normalize horizontally (don't affect Z value)
   {
    float	length, ilength;
    length = v[0]*v[0] + v[1]*v[1];
    if( length )
    {
   	length = sqrt( length );		// FIXME
   	ilength = 1.0f/length;
   	v[0] *= ilength;
   	v[1] *= ilength;
    }
    return length;
   }

   //
   //  walking up a step should kill some velocity
   //


   //==================
   //PM_ClipVelocity -- Kurim : modified for wsw
   //
   //Slide off of the impacting object
   //returns the blocked flags (1 = floor, 2 = step / wall)
   //==================

 #define	STOP_EPSILON	0.1

   static void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
    float	backoff;
    float	change;
    int		i;

    backoff = DotProduct( in, normal );

    if( backoff < 0 ) {
   	backoff *= overbounce;
    } else {
   	backoff /= overbounce;
    }

    for( i = 0; i < 3; i++ ) {
   	change = normal[i]*backoff;
   	out[i] = in[i] - change;
    }
 #ifdef PM_SLIDE_CLAMPING
    {
   	float oldspeed, newspeed;
   	oldspeed = VectorLength( in );
   	newspeed = VectorLength( out );
   	if( newspeed > oldspeed ) {
   	    VectorNormalize( out );
   	    VectorScale( out, oldspeed, out );
   	}
    }
 #endif
   }

   //==================
   //PM_SlideMove
   //
   //Returns a new origin, velocity, and contact entity
   //Does not modify any world state?
   //==================

 #define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
 #define	MAX_CLIP_PLANES	5

 #ifdef NEW_PM_SLIDEMOVE

   static void PM_AddTouchEnt( int entNum ) {
    int		i;

    if( pm->numtouch >= MAXTOUCH )
   	return;

    // see if it is already added
    for( i = 0 ; i < pm->numtouch ; i++ ) {
   	if( pm->touchents[i] == entNum )
   	    return;
    }

    // add it
    pm->touchents[pm->numtouch] = entNum;
    pm->numtouch++;
   }
 #define SLIDEMOVE_PLANEINTERACT_EPSILON	0.1
 #define SLIDEMOVEFLAG_TRAPPED		4
 #define SLIDEMOVEFLAG_BLOCKED		2	// it was blocked at some point, doesn't mean it didn't slide along the blocking object
   static int PM_SlideMove( void )
   {
    vec3_t		end, dir;
    vec3_t		old_velocity, last_valid_origin;
    float		value;
    vec3_t		planes[MAX_CLIP_PLANES];
    int			numplanes = 0;
    trace_t		trace;
    int			moves, i, j, k;
    int			maxmoves = 4;
    float		remainingTime = pml.frametime;
    int			blockedmask = 0;

    VectorCopy( pml.velocity, old_velocity );
    VectorCopy( pml.origin, last_valid_origin );

    if( pm->groundentity != -1 ) { // clip velocity to ground, no need to wait
   	PM_ClipVelocity( pml.velocity, pml.groundplane.normal, pml.velocity, PM_OVERBOUNCE );
    }

    numplanes = 0; // clean up planes count for checking
    // never turn against original velocity. Add it as a clipping plane
    VectorNormalize2( pml.velocity, planes[numplanes] );
    numplanes++;

    for( moves = 0; moves < maxmoves; moves++ )
    {
   	VectorMA( pml.origin, remainingTime, pml.velocity, end );
   	GS_Trace( &trace, pml.origin, pm->mins, pm->maxs, end, pm->passent, pm->contentmask );
   	//pm->trace( &trace, pml.origin, pm->mins, pm->maxs, end );
   	if( trace.allsolid ) {	// trapped into a solid
   	    VectorCopy( last_valid_origin, pml.origin );
   	    return SLIDEMOVEFLAG_TRAPPED;
   	}

   	if( trace.fraction > 0 ) {	// actually covered some distance
   	    VectorCopy( trace.endpos, pml.origin );
   	    VectorCopy( trace.endpos, last_valid_origin );
   	}

   	if( trace.fraction == 1 )
   	    break;		// move done

   	// save touched entity for return output
   	PM_AddTouchEnt( trace.ent );

   	// at this point we are blocked but not trapped.

   	blockedmask |= SLIDEMOVEFLAG_BLOCKED;

   	remainingTime -= ( trace.fraction * remainingTime );

   	// we got blocked, add the plane for sliding along it

   	// if this is a plane we have touched before, try clipping
   	// the velocity along it's normal and repeat.
   	for( i = 0 ; i < numplanes ; i++ ) {
   	    if( DotProduct( trace.plane.normal, planes[i] ) > (1.0f - SLIDEMOVE_PLANEINTERACT_EPSILON) ) {
   		VectorAdd( trace.plane.normal, pml.velocity, pml.velocity );
   		break;
   	    }
   	}
   	if( i < numplanes ) // found a repeated plane, so don't add it, just repeat the trace
   	    continue;

   	// security check: we can't store more planes
   	if( numplanes >= MAX_CLIP_PLANES ) {
   	    VectorClear( pml.velocity );
   	    return SLIDEMOVEFLAG_TRAPPED;
   	}

   	// put the actual plane in the list
   	VectorCopy( trace.plane.normal, planes[numplanes] );
   	numplanes++;

   	//
   	// modify original_velocity so it parallels all of the clip planes
   	//

   	for( i = 0; i < numplanes; i++ ) {
   	    if( DotProduct( pml.velocity, planes[i] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )	// would not touch it
   		continue;

   	    PM_ClipVelocity( pml.velocity, planes[i], pml.velocity, PM_OVERBOUNCE );
   	    // see if we enter a second plane
   	    for( j = 0 ; j < numplanes ; j++ ) {
   		if( j == i ) // it's the same plane
   		    continue;
   		if( DotProduct( pml.velocity, planes[j] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
   		    continue;		// not with this one

   		//there was a second one. Try to slide along it too
   		PM_ClipVelocity( pml.velocity, planes[j], pml.velocity, PM_OVERBOUNCE );

   		// check if the slide sent it back to the first plane
   		if( DotProduct( pml.velocity, planes[i] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
   		    continue;

   		// bad luck: slide the original velocity along the crease
   		CrossProduct( planes[i], planes[j], dir );
   		VectorNormalize( dir );
   		value = DotProduct( dir, pml.velocity );
   		VectorScale( dir, value, pml.velocity );

   		// check if there is a third plane, in that case we're trapped
   		for( k = 0; k < numplanes; k++ ) {
   		    if( j == k || i == k ) // it's the same plane
   			continue;
   		    if( DotProduct( pml.velocity, planes[k] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
   			continue;		// not with this one
   		    VectorClear( pml.velocity );
   		    break;
   		}
   	    }
   	}

   	// if velocity is against the original velocity, stop dead
   	// to avoid tiny oscilations in sloping corners
   	//
   	//if( DotProduct( pml.velocity, old_velocity ) <= 0 ) {
   	    // jalfixme: shouldn't I clip the velocity against the original direction?
   	//	VectorClear( pml.velocity );
   	//	break;
   	//}
    }

    if( pm->s.stats[PM_STAT_NOUSERCONTROL] > 0 ) {
   	VectorCopy( old_velocity, pml.velocity );
    }

    return blockedmask;
   }

 #else
   static int PM_SlideMove( void )
   {
    int			bumpcount, numbumps;
    vec3_t		dir, last_valid_origin;
    float		d;
    int			numplanes;
    vec3_t		planes[MAX_CLIP_PLANES];
    vec3_t		primal_velocity;
    int			i, j;
    trace_t	trace;
    vec3_t		end;
    float		time_left;
    int			blocked = 0;

    numbumps = 4;

    VectorCopy( pml.velocity, primal_velocity );
    VectorCopy( pml.origin, last_valid_origin );
    numplanes = 0;

    time_left = pml.frametime;

    for( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
    {
   	VectorMA( pml.origin, time_left, pml.velocity, end );

   //		pm->trace( &trace, pml.origin, pm->mins, pm->maxs, end );
   	GS_Trace( &trace, pml.origin, pm->mins, pm->maxs, end, pm->passent, pm->contentmask );

   	if( trace.allsolid )
   	{	// entity is trapped in another solid
   	    GS_Printf( "TRAPPED tr.ent %i\n", trace.ent );
   	    //pml.velocity[2] = 0;	// don't build up falling damage
   	    VectorCopy( last_valid_origin, pml.origin );
   	    return 3;
   	}

   	if( trace.fraction > 0 )
   	{	// actually covered some distance
   	    VectorCopy( trace.endpos, last_valid_origin );
   	    VectorCopy( trace.endpos, pml.origin );
   	    numplanes = 0;
   	}

   	if( trace.fraction == 1 )
   	    break;		// moved the entire distance

   	// save entity for contact
   	if( pm->numtouch < MAXTOUCH )
   	{
   	    pm->touchents[pm->numtouch] = trace.ent;
   	    pm->numtouch++;
   	}

   	if( !trace.plane.normal[2] )
   	{
   	    blocked |= 2;		// step
   	}

   	time_left -= time_left * trace.fraction;

   	// slide along this plane
   	if( numplanes >= MAX_CLIP_PLANES )
   	{	// this shouldn't really happen
   	    VectorClear( pml.velocity );
   	    break;
   	}

   	VectorCopy( trace.plane.normal, planes[numplanes] );
   	numplanes++;

   //
   // modify original_velocity so it parallels all of the clip planes
   //
   	for( i = 0; i < numplanes; i++ )
   	{
   	    PM_ClipVelocity( pml.velocity, planes[i], pml.velocity, PM_OVERBOUNCE );
   	    for( j = 0; j < numplanes ; j++ )
   		if( j != i )
   		{
   		    if( DotProduct(pml.velocity, planes[j]) < 0 )
   			break;	// not ok
   		}
   	    if( j == numplanes )
   		break;
   	}

   	if( i != numplanes )
   	{	// go along this plane
   	}
   	else
   	{	// go along the crease
   	    if( numplanes != 2 )
   	    {
   		VectorClear( pml.velocity );
   		break;
   	    }
   	    CrossProduct( planes[0], planes[1], dir );
   	    VectorNormalize( dir );
   	    d = DotProduct( dir, pml.velocity );
   	    VectorScale( dir, d, pml.velocity );
   	}

   	//
   	// if velocity is against the original velocity, stop dead
   	// to avoid tiny occilations in sloping corners
   	//
   	if( DotProduct(pml.velocity, primal_velocity) <= 0 )
   	{
   	    VectorClear( pml.velocity );
   	    break;
   	}
    }

    if( pm->s.stats[PM_STAT_NOUSERCONTROL] > 0 )
    {
   	VectorCopy( primal_velocity, pml.velocity );
    }

    return blocked;
   }
 #endif

   //==================
   //PM_StepSlideMove
   //
   //Each intersection will try to step over the obstruction instead of
   //sliding along it.
   //==================
   static void PM_StepSlideMove( void )
   {
    vec3_t		start_o, start_v;
    vec3_t		down_o, down_v;
    trace_t		trace;
    float		down_dist, up_dist;
    vec3_t		up, down;
    int			blocked;

    VectorCopy( pml.origin, start_o );
    VectorCopy( pml.velocity, start_v );

    blocked = PM_SlideMove();

    VectorCopy( pml.origin, down_o );
    VectorCopy( pml.velocity, down_v );

    VectorCopy( start_o, up );
    up[2] += STEPSIZE;

   //	pm->trace( &trace, up, pm->mins, pm->maxs, up );
    GS_Trace( &trace, up, pm->mins, pm->maxs, up, pm->passent, pm->contentmask );
    if( trace.allsolid )
   	return;		// can't step up

    // try sliding above
    VectorCopy( up, pml.origin );
    VectorCopy( start_v, pml.velocity );

    PM_SlideMove();

    // push down the final amount
    VectorCopy( pml.origin, down );
    down[2] -= STEPSIZE;
   //	pm->trace( &trace, pml.origin, pm->mins, pm->maxs, down );
    GS_Trace( &trace, pml.origin, pm->mins, pm->maxs, down, pm->passent, pm->contentmask );
    if( !trace.allsolid )
    {
   	VectorCopy( trace.endpos, pml.origin );
    }

    VectorCopy( pml.origin, up );

    // decide which one went farther
    down_dist = (down_o[0] - start_o[0])*(down_o[0] - start_o[0])
 + (down_o[1] - start_o[1])*(down_o[1] - start_o[1]);
    up_dist = (up[0] - start_o[0])*(up[0] - start_o[0])
 + (up[1] - start_o[1])*(up[1] - start_o[1]);

    if( down_dist >= up_dist || trace.allsolid || (trace.fraction != 1.0 && trace.plane.normal[2] < MIN_STEP_NORMAL) )
    {
   	VectorCopy( down_o, pml.origin );
   	VectorCopy( down_v, pml.velocity );
   	return;
    }

    if( (blocked & 2) || trace.plane.normal[2] == 1 ) {
   	int stepsize = (int)( pml.origin[2] - pml.previous_origin[2] );
   	if( stepsize > 0 )
   	    pm->step = stepsize ;
    }

    //!! Special case
    // if we were walking along a plane, then we need to copy the Z over
    pml.velocity[2] = down_v[2];
   }

   //==================
   //PM_Friction -- Modified for wsw
   //
   //Handles both ground friction and water friction
   //==================
   static void PM_Friction( void )
   {
    float	*vel;
    float	speed, newspeed, control;
    float	friction;
    float	drop;

    vel = pml.velocity;

    speed = vel[0]*vel[0] +vel[1]*vel[1] + vel[2]*vel[2];
    if( speed < 1 )
    {
   	vel[0] = 0;
   	vel[1] = 0;
   	return;
    }

    speed = sqrt( speed );
    drop = 0;

    // apply ground friction
    if( ( ( ( (pm->groundentity != -1) && !(pml.groundsurfFlags & SURF_SLICK) ) ) && (pm->waterlevel < 2) ) || (pml.ladder) )
    {
   	if( pm->s.stats[PM_STAT_KNOCKBACK] <= 0 )
   	{
   	    friction = pm_friction;
   	    control = speed < pm_stopspeed ? pm_stopspeed : speed;
   	    drop += control*friction*pml.frametime;
   	}
    }

    // apply water friction
    if( (pm->waterlevel >= 2) && !pml.ladder )
   	drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;

    // scale the velocity
    newspeed = speed - drop;
    if( newspeed <= 0 )
    {
   	newspeed = 0;
   	VectorClear( vel );
    }
    else
    {
   	newspeed /= speed;
   	VectorScale( vel, newspeed, vel );
    }
   }

   //==============
   //PM_Accelerate
   //
   //Handles user intended acceleration
   //==============
   static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel )
   {
    int			i;
    float		addspeed, accelspeed, currentspeed;

    currentspeed = DotProduct( pml.velocity, wishdir );
    addspeed = wishspeed - currentspeed;
    if( addspeed <= 0 )
   	return;
    accelspeed = accel*pml.frametime*wishspeed;
    if( accelspeed > addspeed )
   	accelspeed = addspeed;

    for( i = 0; i < 3; i++ )
   	pml.velocity[i] += accelspeed*wishdir[i];
   }

   static void PM_Aircontrol( pmove_t *pm, vec3_t wishdir, float wishspeed )
   {
    float	zspeed, speed, dot, k;
    int		i;
    float	fmove,smove;

    // accelerate
    fmove = pml.fordwardPush;
    smove = pml.sidePush;

    if( (smove > 0 || smove < 0) || (wishspeed == 0.0) )
   	return; // can't control movement if not moveing forward or backward

    zspeed = pml.velocity[2];
    pml.velocity[2] = 0;
    speed = VectorNormalize( pml.velocity );


    dot = DotProduct( pml.velocity,wishdir );
    k = 32;
    k *= pm_aircontrol*dot*dot*pml.frametime;


    if( dot > 0 ) {	// we can't change direction while slowing down
   	for (i=0; i < 2; i++)
   	    pml.velocity[i] = pml.velocity[i]*speed + wishdir[i]*k;
   	VectorNormalize( pml.velocity );
    }

    for( i = 0; i < 2; i++ )
   	pml.velocity[i] *=speed;

    pml.velocity[2] = zspeed;
   }

 #if 0 // never used
   static void PM_AirAccelerate( vec3_t wishdir, float wishspeed, float accel )
   {
    int			i;
    float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;

    if( wishspd > 30 )
   	wishspd = 30;
    currentspeed = DotProduct( pml.velocity, wishdir );
    addspeed = wishspd - currentspeed;
    if( addspeed <= 0 )
   	return;
    accelspeed = accel * wishspeed * pml.frametime;
    if( accelspeed > addspeed )
   	accelspeed = addspeed;

    for( i = 0; i < 3; i++ )
   	pml.velocity[i] += accelspeed*wishdir[i];
   }
 #endif


   //=============
   //PM_AddCurrents
   //=============
   static void PM_AddCurrents( vec3_t wishvel )
   {
    //
    // account for ladders
    //

    if( pml.ladder && fabs(pml.velocity[2]) <= 200 )
    {
   	if( (pm->viewangles[PITCH] <= -15) && (pml.fordwardPush > 0) )
   	    wishvel[2] = 200;
   	else if( (pm->viewangles[PITCH] >= 15) && (pml.fordwardPush > 0) )
   	    wishvel[2] = -200;
   	else if( pml.upPush > 0 )
   	    wishvel[2] = 200;
   	else if( pml.upPush < 0 )
   	    wishvel[2] = -200;
   	else
   	    wishvel[2] = 0;

   	// limit horizontal speed when on a ladder
   	clamp( wishvel[0], -25, 25 );
   	clamp( wishvel[1], -25, 25 );
    }
   }

   //===================
   //PM_WaterMove
   //
   //===================
   static void PM_WaterMove( void )
   {
    int		i;
    vec3_t	wishvel;
    float	wishspeed;
    vec3_t	wishdir;

    // user intentions
    for( i = 0; i < 3; i++ )
   	wishvel[i] = pml.viewAxis[FORWARD][i]*pml.fordwardPush + pml.viewAxis[RIGHT][i]*pml.sidePush;

    if( !pml.fordwardPush && !pml.sidePush && !pml.upPush )
   	wishvel[2] -= 60;		// drift towards bottom
    else
   	wishvel[2] += pml.upPush;

    PM_AddCurrents( wishvel );

    VectorCopy( wishvel, wishdir );
    wishspeed = VectorNormalize( wishdir );

    if( wishspeed > pm_maxspeed )
    {
   	wishspeed = pm_maxspeed/wishspeed;
   	VectorScale( wishvel, wishspeed, wishvel );
   	wishspeed = pm_maxspeed;
    }
    wishspeed *= 0.5;

    PM_Accelerate( wishdir, wishspeed, pm_wateraccelerate );
    PM_StepSlideMove();
   }

   //===================
   //PM_AirMove -- Kurim
   //
   //===================
   static void PM_AirMove( void )
   {
    int			i;
    vec3_t		wishvel;
    vec3_t		wishdir;
    float		wishspeed;
    float		maxspeed;
    float		accel;
    float		wishspeed2;

    for( i = 0; i < 2; i++ )
   	wishvel[i] = pml.viewAxis[FORWARD][i]*pml.fordwardPush + pml.viewAxis[RIGHT][i]*pml.sidePush;
    wishvel[2] = 0;

    PM_AddCurrents( wishvel );

    VectorCopy( wishvel, wishdir );
    wishspeed = VectorNormalize( wishdir );

    // clamp to server defined max speed

    // wsw : jal : fixme?: shall we use a pmflag?
    if( pm->s.pm_flags & PMF_DUCKED ) {
   	maxspeed = pm_duckspeed;
    } else if( pm->cmd.buttons & BUTTON_WALK ) {
   	maxspeed = pm_maxwalkspeed;
    } else
   	maxspeed = pm_maxspeed;

    if( wishspeed > maxspeed )
    {
   	wishspeed = maxspeed/wishspeed;
   	VectorScale( wishvel, wishspeed, wishvel );
   	wishspeed = maxspeed;
    }

    if( pml.ladder )
    {
   	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

   	if( !wishvel[2] )
   	{
   	    if( pml.velocity[2] > 0 )
   	    {
   		pml.velocity[2] -= pm->gravity * pml.frametime;
   		if( pml.velocity[2] < 0 )
   		    pml.velocity[2]  = 0;
   	    }
   	    else
   	    {
   		pml.velocity[2] += pm->gravity * pml.frametime;
   		if( pml.velocity[2] > 0 )
   		    pml.velocity[2]  = 0;
   	    }
   	}

   	PM_StepSlideMove();
    }
    else if( pm->groundentity != -1 )
    {	// walking on ground
   	if( pml.velocity[2] > 0 )
   	    pml.velocity[2] = 0; //!!! this is before the accel
   	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

   	// fix for negative trigger_gravity fields
   	if( pm->gravity > 0 ) {
   	    if( pml.velocity[2] > 0 )
   		pml.velocity[2] = 0;
   	} else
   	    pml.velocity[2] -= pm->gravity * pml.frametime;

   	if( !pml.velocity[0] && !pml.velocity[1] )
   	    return;

   	PM_StepSlideMove();
    }
    else

    // Kurim : changing : THIS :
    //{	// not on ground, so little effect on velocity
    //	if (pm_airaccelerate)
    //		PM_AirAccelerate (wishdir, wishspeed, pm_accelerate);
    //	else
    //		PM_Accelerate (wishdir, wishspeed, 1);

    //	// add gravity
    //	pml.velocity[2] -= pm->s.gravity * pml.frametime;
    //	PM_StepSlideMove ();
    //}

    // TO THIS :
    {
   	// Air Control
   	wishspeed2 = wishspeed;
   	if( DotProduct(pml.velocity, wishdir) < 0 )
   	    accel = pm_airstopaccelerate;
   	else
   	    accel = 1; //instead of pm_airaccelerate, (correcting the forward bug);

   	if( (pml.sidePush > 0 || pml.sidePush < 0) && pml.fordwardPush == 0 )
   	{
   	    if( wishspeed > pm_wishspeed )
   		wishspeed = pm_wishspeed;
   	    accel = pm_strafeaccelerate;
   	}
   	// Air control
   	PM_Accelerate( wishdir, wishspeed, accel );
   	if( pm_aircontrol ) // no air ctrl while wjing
   	    PM_Aircontrol( pm, wishdir, wishspeed2 );

   	// add gravity
   	pml.velocity[2] -= pm->gravity * pml.frametime;
   	PM_StepSlideMove();
    }
   }


   //=============
   //PM_CategorizePosition
   //=============
   //#define JITSPOES_CLIPBUGFIX // jal : it bugs more than fixes?
   static void PM_CategorizePosition( void )
   {
    vec3_t		point;
    int			cont;
    trace_t		trace;
    int			sample1;
    int			sample2;

    // if the player hull point one-quarter unit down is solid, the player is on ground

    // see if standing on something solid
    point[0] = pml.origin[0];
    point[1] = pml.origin[1];
    point[2] = pml.origin[2] - 0.25;

    if( pml.velocity[2] > 180 ) // !!ZOID changed from 100 to 180 (ramp accel)
    {
   	pm->s.pm_flags &= ~PMF_ON_GROUND;
   	pm->groundentity = -1;
    }
    else
    {
   //		pm->trace( &trace, pml.origin, pm->mins, pm->maxs, point );
   	GS_Trace( &trace, pml.origin, pm->mins, pm->maxs, point, pm->passent, pm->contentmask );
   	pml.groundplane = trace.plane;
   	pml.groundsurfFlags = trace.surfaceFlags;
   	pml.groundcontents = trace.contents;

   	if( (trace.fraction == 1) || (trace.plane.normal[2] < 0.7 && !trace.startsolid) )
   	{
 #ifdef JITSPOES_CLIPBUGFIX
   	    trace_t      trace2;
   	    vec3_t      mins, maxs;

   	    // try a slightly smaller bounding box -- this is to fix getting stuck up
   	    // on angled walls and not being able to move (like you're stuck in the air)
   	    mins[0] = pm->mins[0] ? pm->mins[0] + 1 : 0;
   	    mins[1] = pm->mins[1] ? pm->mins[1] + 1 : 0;
   	    mins[2] = pm->mins[2];
   	    maxs[0] = pm->maxs[0] ? pm->maxs[0] - 1 : 0;
   	    maxs[1] = pm->maxs[1] ? pm->maxs[1] - 1 : 0;
   	    maxs[2] = pm->maxs[2];
   	    pm->trace( &trace2, pml.origin, mins, maxs, point );

   	    if( !(trace2.plane.normal[2] < 0.7f && !trace2.startsolid) )
   	    {
   		memcpy( &trace, &trace2, sizeof(trace) );
   		pml.groundplane = trace.plane;
   		pml.groundsurfFlags = trace.surfFlags;
   		pml.groundcontents = trace.contents;
   		pm->groundentity = trace.ent;
   	    }
 #else //JITSPOES_CLIPBUGFIX
   	    pm->groundentity = -1;
   	    pm->s.pm_flags &= ~PMF_ON_GROUND;
 #endif //JITSPOES_CLIPBUGFIX
   	}
   	else
   	{
   	    pm->groundentity = trace.ent;

   	    if( !(pm->s.pm_flags & PMF_ON_GROUND) )
   	    {	// just hit the ground
   		pm->s.pm_flags |= PMF_ON_GROUND;
   	    }
   	}

   	if( (pm->numtouch < MAXTOUCH) && (trace.fraction < 1.0) )
   	{
   	    pm->touchents[pm->numtouch] = trace.ent;
   	    pm->numtouch++;
   	}
    }

   //
   // get waterlevel, accounting for ducking
   //
    pm->waterlevel = 0;
    pm->watertype = 0;

    sample2 = pm->viewheight - pm->mins[2];
    sample1 = sample2 / 2;

    point[2] = pml.origin[2] + pm->mins[2] + 1;
   //	cont = pm->pointcontents( point );
    cont = GS_PointContents( point );

    if( cont & MASK_WATER )
    {
   	pm->watertype = cont;
   	pm->waterlevel = 1;
   	point[2] = pml.origin[2] + pm->mins[2] + sample1;
   //		cont = pm->pointcontents( point );
   	cont = GS_PointContents( point );
   	if( cont & MASK_WATER )
   	{
   	    pm->waterlevel = 2;
   	    point[2] = pml.origin[2] + pm->mins[2] + sample2;
   //			cont = pm->pointcontents( point );
   	    cont = GS_PointContents( point );
   	    if( cont & MASK_WATER )
   		pm->waterlevel = 3;
   	}
    }

   }

   //=============
   //PM_CheckJump
   //=============
   static void PM_CheckJump( void )
   {
    if( pml.upPush < 10 )
    {	// not holding jump
   	pm->s.pm_flags &= ~PMF_JUMP_HELD;
   	return;
    }

    // must wait for jump to be released
    if( pm->s.pm_flags & PMF_JUMP_HELD )
   	return;

    if( pm->s.pm_type >= PM_DEAD )
   	return;

    if( pm->waterlevel >= 2 )
    {	// swimming, not jumping
   	pm->groundentity = -1;
   	return;
    }

    if( pm->groundentity == -1 )
   	return;

    pm->s.pm_flags |= PMF_JUMP_HELD;
    pm->groundentity = -1;
    if (pml.velocity[2] > 0)
   	pml.velocity[2] += pm_jump_velocity;
    else
   	pml.velocity[2] = pm_jump_velocity;

    // return sound events
    pm->events |= PMEV_JUMPED;
   }

   //=============
   //PM_CheckSpecialMovement
   //=============
   static void PM_CheckSpecialMovement( void )
   {
    vec3_t	spot;
    int		cont;
    vec3_t	flatforward;
    trace_t	trace;

    if( pm->s.stats[PM_STAT_NOUSERCONTROL] > 0 )
   	return;

    pml.ladder = qfalse;

    // check for ladder
    flatforward[0] = pml.viewAxis[FORWARD][0];
    flatforward[1] = pml.viewAxis[FORWARD][1];
    flatforward[2] = 0;
    VectorNormalize( flatforward );

    VectorMA( pml.origin, 1, flatforward, spot );
   //	pm->trace( &trace, pml.origin, pm->mins, pm->maxs, spot );
    GS_Trace( &trace, pml.origin, pm->mins, pm->maxs, spot, pm->passent, pm->contentmask );
    if( (trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER) )
   	pml.ladder = qtrue;

    // check for water jump
    if( pm->waterlevel != 2 )
   	return;

    VectorMA( pml.origin, 30, flatforward, spot );
    spot[2] += 4;
   //	cont = pm->pointcontents( spot );
    cont = GS_PointContents( spot );
    if( !(cont & CONTENTS_SOLID) )
   	return;

    spot[2] += 16;
   //	cont = pm->pointcontents( spot );
    cont = GS_PointContents( spot );
    if( cont )
   	return;
    // jump out of water
    VectorScale( flatforward, 50, pml.velocity );
    pml.velocity[2] = 350;

    pm->s.stats[PM_STAT_NOUSERCONTROL] = 255;
   }

   //===============
   //PM_FlyMove
   //===============
   static void PM_FlyMove( qboolean doclip )
   {
    float		speed, drop, friction, control, newspeed;
    float		currentspeed, addspeed, accelspeed;
    int			i;
    vec3_t		wishvel;
    vec3_t		wishdir;
    float		wishspeed;
    vec3_t		end;
    trace_t		trace;

    pm->viewheight = playerbox_stand_viewheight;

    // friction

    speed = VectorLength( pml.velocity );
    if( speed < 1 )
    {
   	VectorClear( pml.velocity );
    }
    else
    {
   	drop = 0;

   	friction = pm_friction*1.5;	// extra friction
   	control = speed < pm_stopspeed ? pm_stopspeed : speed;
   	drop += control*friction*pml.frametime;

   	// scale the velocity
   	newspeed = speed - drop;
   	if( newspeed < 0 )
   	    newspeed = 0;
   	newspeed /= speed;

   	VectorScale( pml.velocity, newspeed, pml.velocity );
    }

    // accelerate
    VectorNormalize( pml.viewAxis[FORWARD] );
    VectorNormalize( pml.viewAxis[RIGHT] );

    for( i = 0; i < 3; i++ )
   	wishvel[i] = pml.viewAxis[FORWARD][i]*pml.fordwardPush + pml.viewAxis[RIGHT][i]*pml.sidePush;

    wishvel[2] += pml.upPush;

    VectorCopy( wishvel, wishdir );
    wishspeed = VectorNormalize( wishdir );

    //
    // clamp to server defined max speed
    //
    if( wishspeed > pm_maxspeed )
    {
   	wishspeed = pm_maxspeed/wishspeed;
   	VectorScale( wishvel, wishspeed, wishvel );
   	wishspeed = pm_maxspeed;
    }

    currentspeed = DotProduct( pml.velocity, wishdir );
    addspeed = wishspeed - currentspeed;
    if( addspeed <= 0 )
   	return;
    accelspeed = pm_accelerate*pml.frametime*wishspeed;
    if( accelspeed > addspeed )
   	accelspeed = addspeed;

    for( i=0 ; i<3 ; i++ )
   	pml.velocity[i] += accelspeed*wishdir[i];

    if( doclip ) {
   	for( i = 0; i < 3; i++ )
   	    end[i] = pml.origin[i] + pml.frametime * pml.velocity[i];

   //		pm->trace( &trace, pml.origin, pm->mins, pm->maxs, end );
   	GS_Trace( &trace, pml.origin, pm->mins, pm->maxs, end, pm->passent, pm->contentmask );

   	VectorCopy( trace.endpos, pml.origin );
    } else {
   	// move
   	VectorMA( pml.origin, pml.frametime, pml.velocity, pml.origin );
    }
   }

   //==============
   //PM_CheckDuck
   //
   //Sets mins, maxs, and pm->viewheight
   //==============
   static void PM_CheckDuck( void )
   {
    trace_t	trace;
    vec3_t	end;


    //pm->mins[0] = playerbox_stand_mins[0];
    //pm->mins[1] = playerbox_stand_mins[1];

    //pm->maxs[0] = playerbox_stand_maxs[0];
    //pm->maxs[1] = playerbox_stand_maxs[1];

    VectorCopy( playerbox_stand_mins, pm->mins );
    VectorCopy( playerbox_stand_maxs, pm->maxs );

    if( pm->s.pm_type == PM_GIB )
    {
   	//VectorCopy( playerbox_gib_maxs, pm->maxs );
   	//VectorCopy( playerbox_gib_mins, pm->mins );
   	pm->mins[2] = playerbox_gib_mins[2];
   	pm->maxs[2] = playerbox_gib_maxs[2];
   	pm->viewheight = playerbox_gib_viewheight;
   	return;
    }

    pm->mins[2] = playerbox_stand_mins[2];

    if( pm->s.pm_type >= PM_DEAD )
    {
   	//VectorCopy( playerbox_stand_maxs, pm->maxs );
   	//VectorCopy( playerbox_stand_mins, pm->mins );

   	pm->maxs[2] = -8;
   	pm->viewheight = 8;
   	return;
    }

    // wsw: pb allow player to duck while in air
    if( pml.upPush < 0 )//&& (pm->s.pm_flags & PMF_ON_GROUND) )
    {	// duck
   	pm->s.pm_flags |= PMF_DUCKED;
    }
    else
    {	// stand up if possible
   	if( pm->s.pm_flags & PMF_DUCKED )
   	{
   	    // try to stand up
   	    VectorCopy( pml.origin, end );
   	    end[2] += playerbox_stand_maxs[2] - pm->maxs[2];
   //			pm->trace( &trace, pml.origin, pm->mins, pm->maxs, end );
   	    GS_Trace( &trace, pml.origin, pm->mins, pm->maxs, end, pm->passent, pm->contentmask );
   	    if( trace.fraction == 1 ) {
   		pm->maxs[2] = playerbox_stand_maxs[2];
   		pm->s.pm_flags &= ~PMF_DUCKED;
   	    }
   	}
    }

    if( pm->s.pm_flags & PMF_DUCKED )
    {
   	//VectorCopy( playerbox_crouch_mins, pm->mins );
   	//VectorCopy( playerbox_crouch_maxs, pm->maxs );
   	pm->maxs[2] = playerbox_crouch_maxs[2];
   	pm->viewheight = playerbox_crouch_viewheight;
    }
    else
    {
   	//VectorCopy( playerbox_stand_mins, pm->mins );
   	//VectorCopy( playerbox_stand_maxs, pm->maxs );
   	pm->maxs[2] = playerbox_stand_maxs[2];
   	pm->viewheight = playerbox_stand_viewheight;
    }
   }

   //==============
   //PM_DeadMove
   //==============
   static void PM_DeadMove( void )
   {
    float	forward;

    if( pm->groundentity == -1 )
   	return;

    // extra friction
    forward = VectorLength( pml.velocity ) - 20;

    if( forward <= 0 )
    {
   	VectorClear( pml.velocity );
    }
    else
    {
   	VectorNormalize( pml.velocity );
   	VectorScale( pml.velocity, forward, pml.velocity );
    }
   }

 #ifndef FLOATPMOVE
   static qboolean PM_GoodPosition( int snaptorigin[3] )
   {
    trace_t	trace;
    vec3_t	origin, end;
    int		i;

    if( pm->s.pm_type == PM_SPECTATOR )
   	return qtrue;

    for( i = 0; i < 3; i++ )
   	origin[i] = end[i] = snaptorigin[i]*(1.0/PM_VECTOR_SNAP);

    GS_Trace( &trace, origin, pm->mins, pm->maxs, end, pm->passent, pm->contentmask );

    return !trace.allsolid;
   }
 #endif

   //================
   //PM_SnapPosition
   //
   //On exit, the origin will have a value that is pre-quantized to the (1.0/16.0)
   //precision of the network channel and in a valid position.
   //================
   static void PM_SnapPosition( void )
   {
 #ifdef FLOATPMOVE
    VectorCopy( pml.origin, pm->s.origin );
    VectorCopy( pml.velocity, pm->s.velocity );
    GS_SnapVelocity( pm->s.velocity	);
 #else
    int		sign[3];
    int		i, j, bits;
    int		base[3];
    int		velint[3], origint[3];
    // try all single bits first
    static const int jitterbits[8] = {0,4,1,2,3,5,6,7};

    // snap velocity to sixteenths
    for( i = 0; i < 3; i++ ) {
   	velint[i] = (int)(pml.velocity[i]*16);
   	pm->s.velocity[i] = velint[i]*(1.0/16.0);
    }

    for( i = 0; i < 3; i++ )
    {
   	if( pml.origin[i] >= 0 )
   	    sign[i] = 1;
   	else
   	    sign[i] = -1;
   	origint[i] = (int)(pml.origin[i]*16);
   	if( origint[i]*(1.0/16.0) == pml.origin[i] )
   	    sign[i] = 0;
    }
    VectorCopy( origint, base );

    // try all combinations
    for( j = 0; j < 8; j++ )
    {
   	bits = jitterbits[j];
   	VectorCopy( base, origint );
   	for( i = 0; i < 3; i++ )
   	    if( bits & (1<<i) )
   		origint[i] += sign[i];

   	if( PM_GoodPosition(origint) ) {
   	    VectorScale( origint, (1.0/16.0), pm->s.origin );
   	    return;
   	}
    }

    // go back to the last position
    VectorCopy( pml.previous_origin, pm->s.origin );
    VectorClear( pm->s.velocity );
 #endif
   }

 #ifndef FLOATPMOVE
   //================
   //PM_InitialSnapPosition
   //
   //================
   static void PM_InitialSnapPosition( void )
   {
    int        x, y, z;
    int	       base[3];
    static const int offset[3] = { 0, -1, 1 };
    int			origint[3];

    VectorScale( pm->s.origin, 16, origint );
    VectorCopy( origint, base );

    for( z = 0; z < 3; z++ ) {
   	origint[2] = base[2] + offset[ z ];
   	for( y = 0; y < 3; y++ ) {
   	    origint[1] = base[1] + offset[ y ];
   	    for( x = 0; x < 3; x++ ) {
   		origint[0] = base[0] + offset[ x ];
   		if( PM_GoodPosition(origint) ) {
   		    pml.origin[0] = pm->s.origin[0] = origint[0]*(1.0/16.0);
   		    pml.origin[1] = pm->s.origin[1] = origint[1]*(1.0/16.0);
   		    pml.origin[2] = pm->s.origin[2] = origint[2]*(1.0/16.0);
   		    VectorCopy( pm->s.origin, pml.previous_origin );
   		    return;
   		}
   	    }
   	}
    }
   }
 #endif

   //================
   //PM_ClampAngles
   //
   //================
   static void PM_ClampAngles( void )
   {
    int		i;
    short	temp;

    for( i = 0; i < 3; i++ )
    {
   	temp = pm->cmd.angles[i] + pm->s.delta_angles[i];
   	if ( i == PITCH ) {
   	    // don't let the player look up or down more than 90 degrees
   	    if ( temp > 16000 ) {
   		pm->s.delta_angles[i] = 16000 - pm->cmd.angles[i];
   		temp = 16000;
   	    } else if ( temp < -16000 ) {
   		pm->s.delta_angles[i] = -16000 - pm->cmd.angles[i];
   		temp = -16000;
   	    }
   	}

   	pm->viewangles[i] = SHORT2ANGLE( temp );
    }

    AngleVectors( pm->viewangles, pml.viewAxis[FORWARD], pml.viewAxis[RIGHT], pml.viewAxis[UP] );
   }

   //================
   //Pmove
   //
   //Can be called by either the server or the client
   //================
   void Pmove( pmove_t *pmove )
   {
    pm = pmove;

    // clear results
    pm->numtouch = 0;
    VectorClear( pm->viewangles );
    pm->viewheight = 0;
    pm->groundentity = -1;
    pm->watertype = 0;
    pm->waterlevel = 0;
    pm->step = qfalse;

    // clear all pmove local vars
    memset( &pml, 0, sizeof(pml) );

    VectorCopy( pm->s.origin, pml.origin );
    VectorCopy( pm->s.velocity, pml.velocity );

    // save old org in case we get stuck
    VectorCopy( pm->s.origin, pml.previous_origin );

    pml.frametime = pm->cmd.msec * 0.001;

    pml.fordwardPush = pm->cmd.forwardfrac * pm->cmd.msec * SPEEDKEY;
    pml.sidePush = pm->cmd.sidefrac * pm->cmd.msec * SPEEDKEY;
    pml.upPush = pm->cmd.upfrac * pm->cmd.msec * SPEEDKEY;

    PM_ClampAngles();

    if( pm->s.pm_type == PM_SPECTATOR ) {
   	PM_FlyMove( qfalse );
   	PM_SnapPosition();
   	return;
    }

    if( pm->s.pm_type >= PM_DEAD ) {
   	pml.fordwardPush = 0;
   	pml.sidePush = 0;
   	pml.upPush = 0;
    }

    if( pm->s.pm_type >= PM_FREEZE ) // includes PM_CHASECAM
   	return;		// no movement at all

    // set mins, maxs, and viewheight
    PM_CheckDuck();
 #ifndef FLOATPMOVE
    if( pm->snapinitial )
   	PM_InitialSnapPosition();
 #endif
    // set groundentity, watertype, and waterlevel
    PM_CategorizePosition();

    if( pm->s.pm_type == PM_DEAD )
   	PM_DeadMove();

    PM_CheckSpecialMovement();

    if( pm->s.stats[PM_STAT_KNOCKBACK] > 0 )
   	pm->s.stats[PM_STAT_KNOCKBACK] -= pm->cmd.msec;
    if( pm->s.stats[PM_STAT_NOUSERCONTROL] > 0 )
   	pm->s.stats[PM_STAT_NOUSERCONTROL] -= pm->cmd.msec;

    {
   	// Kurim
   	// Keep this order !
   	PM_CheckJump();

   	PM_Friction();

   	if( pm->waterlevel >= 2 ) {
   	    PM_WaterMove();
   	}
   	else {
   	    vec3_t	angles;

   	    VectorCopy( pm->viewangles, angles );
   	    if ( angles[PITCH] > 180 )
   		angles[PITCH] = angles[PITCH] - 360;
   	    angles[PITCH] /= 3;

   	    AngleVectors( angles, pml.viewAxis[FORWARD], pml.viewAxis[RIGHT], pml.viewAxis[UP] );
   	    PM_AirMove();
   	}
    }

    // set groundentity, watertype, and waterlevel for final spot
    PM_CategorizePosition();
    PM_SnapPosition();
   }
 */
