/*
   Copyright (C) 2007 German Garcia
 */

#include "gs_local.h"

//==================================================
// COLLISION MODELS
//==================================================

/*
* GS_DecodeEntityBBox
*/
void GS_DecodeEntityBBox( int bbox, vec3_t mins, vec3_t maxs )
{
	int r, zd, zu;

	zu = 8 * ( bbox & 31 );
	zd = 8 * ( ( bbox>>5 ) & 31 );
	r = 8 * ( ( bbox>>10 ) & 63 );

	if( !r )
	{
		mins[0] = mins[1] = maxs[0] = maxs[1] = 0.0f;
	}
	else
	{
		mins[0] = mins[1] = -r;
		maxs[0] = maxs[1] = r;
	}
	mins[2] = ( zd ) ? -zd : 0.0f;
	maxs[2] = zu;
}

/*
* GS_EncodeEntityBBox
* - x and y, mins and maxs have to be the same (so, radius), in a range from 0 to 504 (in 8 unit steps)
* - any of radius (x, y), z mins and z maxs can be zero.
* - z mins and z maxs can be different, in a range from 0 to 248 (in 8 unit steps).
* - no max value can be negative nor mins value positive.
*/
int GS_EncodeEntityBBox( vec3_t mins, vec3_t maxs )
{
	int r, zd, zu;
	int bbox;

	// z is not symmetric
	zd = (int)( abs( mins[2] )/8 );
	clamp( zd, 0, 31 );

	// and z maxs can be negative...
	zu = (int)( maxs[2]/8 );
	clamp( zu, 0, 31 );

	// assume that x/y are equal and symmetric
	r = (int)( maxs[0]/8 );
	clamp( r, 0, 63 );

	bbox = ( r<<10 ) | ( zd<<5 ) | zu;

	// convert mins and maxs back from the encoded into the full precision value
	GS_DecodeEntityBBox( bbox, mins, maxs );

	return bbox;
}

qboolean GS_IsBrushModel( int modelindex )
{
	return ( qboolean )( ( modelindex > 0 ) && ( (int)modelindex < module.trap_CM_NumInlineModels() ) );
}

/*
* GS_CModelForEntity
* returns a cmodel with the collision model for the given entity.
* It doesn't matter if it's brushmodel or bbox.
*/
struct cmodel_s *GS_CModelForEntity( entity_state_t *state )
{
	struct cmodel_s	*cmodel;
	vec3_t bmins, bmaxs;

	if( !state || state->number < 0 || state->number >= MAX_EDICTS )
		return NULL;

	if( !state->solid )  // it's not interacting
		return NULL;

	// find the cmodel
	if( state->cmodeltype == CMODEL_BRUSH )
	{                                       // special value for bmodel
		cmodel = module.trap_CM_InlineModel( state->modelindex1 );
	}
	else
	{                                   // encoded bbox
		GS_DecodeEntityBBox( state->bbox, bmins, bmaxs );
		cmodel = module.trap_CM_ModelForBBox( bmins, bmaxs );
	}

	return cmodel;
}

/*
* GS_AbsoluteBoundsForEntity - absmins and absmaxs are absolute, axis aligned, containing the (maybe) rotated box.
*/
void GS_AbsoluteBoundsForEntity( entity_state_t *state )
{
	float radius;
	int i;

	// when cmodel is rotated, find the bounding box containing the rotated box
	if( ( state->cmodeltype == CMODEL_BRUSH || state->cmodeltype == CMODEL_BBOX_ROTATED )
	   && ( state->ms.angles[0] || state->ms.angles[1] || state->ms.angles[2] ) )
	{
		// current orientation bounds. Precise bounding box for current orientation
		vec3_t axis[3], localAxis[3];
		vec3_t points[8], point;
		AnglesToAxis( state->ms.angles, axis );
		Matrix_Transpose( axis, localAxis );

		BuildBoxPoints( points, vec3_origin, state->local.mins, state->local.maxs );
		for( i = 0; i < 8; i++ )
		{
			Matrix_TransformVector( localAxis, points[i], point );
			AddPointToBounds( point, state->local.boundmins, state->local.boundmaxs );
		}

		// potentially touching bounds for any orientation
		radius = RadiusFromBounds( state->local.mins, state->local.maxs );
		for( i = 0; i < 3; i++ )
		{
			state->local.absmins[i] = state->ms.origin[i] - radius;
			state->local.absmaxs[i] = state->ms.origin[i] + radius;
		}
	}
	else
	{   
		// when not rotated, the box is already bounding
		VectorCopy( state->local.mins, state->local.boundmins );
		VectorCopy( state->local.maxs, state->local.boundmaxs );
		VectorAdd( state->ms.origin, state->local.mins, state->local.absmins );
		VectorAdd( state->ms.origin, state->local.maxs, state->local.absmaxs );
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
#if 0
	absmins[0] -= 1;
	absmins[1] -= 1;
	absmins[2] -= 1;
	absmaxs[0] += 1;
	absmaxs[1] += 1;
	absmaxs[2] += 1;
#endif
}

/*
* GS_CenterOfEntity
*/
void GS_CenterOfEntity( vec3_t currentOrigin, entity_state_t *state, vec3_t center )
{
	int i;

	if( !state->solid || !state->cmodeltype || ( state->cmodeltype != CMODEL_BRUSH && !state->bbox ) )
	{
		VectorCopy( currentOrigin, center );
		return;
	}

	for( i = 0; i < 3; i++ )
		center[i] = currentOrigin[i] + ( 0.5f * ( state->local.maxs[i] + state->local.mins[i] ) );
}

/*
* GS_RelativeBoxForBrushModel
* Used to create trigger bboxes from their target entity brushmodel.
* box_origin acts as entity origin input and output.
*/
void GS_RelativeBoxForBrushModel( struct cmodel_s *cmodel, vec3_t box_origin, vec3_t box_mins, vec3_t box_maxs )
{
	vec3_t size, box_absmin, box_absmax;

	if( !cmodel )
		GS_Error( "G_RelativeBoxForBrushModel: Entity without a brush model\n" );

	module.trap_CM_InlineModelBounds( cmodel, box_absmin, box_absmax );
	VectorAdd( box_absmin, box_origin, box_absmin );
	VectorAdd( box_absmax, box_origin, box_absmax );
	VectorSubtract( box_absmax, box_absmin, size );
	VectorScale( size, 0.5, size );
	VectorAdd( box_absmin, size, box_origin );
	box_mins[0] = -size[0];
	box_mins[1] = -size[1];
	box_mins[2] = -size[2];
	box_maxs[0] = size[0];
	box_maxs[1] = size[1];
	box_maxs[2] = size[2];
}

//==================================================
// CROSSHAIR ACTIVATION OF ENTITIES
//==================================================

/*
* GS_FindActivateTargetInFront
*/
int GS_FindActivateTargetInFront( entity_state_t *state, player_state_t *playerState, int timeDelta )
{
	trace_t	trace;
	int mask = MASK_OPAQUE|MASK_PLAYERSOLID|CONTENTS_TRIGGER;
	int range = 128;
	vec3_t start, end, viewdir;

	if( !GS_IsGhostState( state ) )
	{
		start[0] = state->ms.origin[0];
		start[1] = state->ms.origin[1];
		start[2] = state->ms.origin[2] + playerState->viewHeight;

		AngleVectors( state->ms.angles, viewdir, NULL, NULL );
		VectorMA( start, range, viewdir, end );

		GS_Trace( &trace, start, vec3_origin, vec3_origin, end, state->number, mask, timeDelta );
		if( trace.ent != ENTITY_INVALID && trace.ent != ENTITY_WORLD )
		{
			entity_state_t *target;
			target = module.GetClipStateForDeltaTime( trace.ent, timeDelta );
			if( !target )
				return ENTITY_INVALID;
			if( target->team && ( target->team != state->team ) )
				return ENTITY_INVALID;
			if( target->flags & SFL_ACTIVABLE )
				return target->number;
		}
	}

	return ENTITY_INVALID;
}

//==================================================
// INSTANT HIT BULLET TRACING
//==================================================

/*
* GS_SeedFire_PosForPattern
*/
void GS_SpreadDir( vec3_t inDir, vec3_t outDir, int hspread, int vspread, int *seed )
{
	double alpha, s;
	float r, u;
	vec3_t axis[3];
	int pattern = 0;

	switch( pattern )
	{
	default:
	case 0:
		// circle
		alpha = M_PI * Q_crandom( seed );
		s = fabs( Q_crandom( seed ) );
		r = s * cos( alpha ) * hspread;
		u = s * sin( alpha ) * vspread;
		break;
	case 1:
		// square
		r = Q_crandom( seed ) * hspread;
		u = Q_crandom( seed ) * vspread;
		break;
	}

	VectorNormalize2( inDir, outDir );
	NormalVectorToAxis( outDir, axis );

	VectorMA( vec3_origin, SPREAD_CALCULATION_RANGE, axis[FORWARD], outDir );
	VectorMA( outDir, r, axis[RIGHT], outDir );
	VectorMA( outDir, u, axis[UP], outDir );

	VectorNormalizeFast( outDir );
}

/*
* GS_SeedFireBullet
*/
#define BULLET_WATER_REFRACTION 400
void GS_SeedFireBullet( trace_t *trace, vec3_t start, vec3_t dir, int range,
                       int hspread, int vspread, int *seed, int ignore, int timeDelta,
                       void ( *impactEvent )( vec3_t frstart, trace_t *trace, qboolean water, qboolean transition ) )
{
	vec3_t spreadDir, end;
	int contentmask = ( MASK_SHOT | MASK_WATER );
	qboolean water = qfalse;
	int refraction = 0;

	hspread *= 8;
	vspread *= 8;

	// see if we start at liquid
	if( GS_PointContents( start, timeDelta ) & MASK_WATER )
	{
		water = qtrue;
		refraction = BULLET_WATER_REFRACTION;
		contentmask &= ~MASK_WATER;
	}

	// do the spread offsetting
	GS_SpreadDir( dir, spreadDir, hspread + refraction, vspread + refraction, seed );
	VectorMA( start, range, spreadDir, end );

	GS_Trace( trace, start, vec3_origin, vec3_origin, end, ignore, contentmask, timeDelta );

	// if starting from water no need to modify the refraction afterwards, so finish
	if( water )
	{
		if( impactEvent )
		{
			if( !( GS_PointContents( trace->endpos, timeDelta ) & MASK_WATER ) )
			{
				trace_t	tr;
				contentmask |= MASK_WATER;
				GS_Trace( &tr, trace->endpos, vec3_origin, vec3_origin, start, ignore, contentmask, timeDelta );
				contentmask &= ~MASK_WATER;
				if( !VectorCompare( start, tr.endpos ) /*&& (tr.contents & MASK_WATER)*/ )
				{
					// found water exit
					impactEvent( start, &tr, water, qtrue );
				}

				impactEvent( tr.endpos, trace, qfalse, qfalse );
				return;
			}

			// the full trace happens underwater
			impactEvent( start, trace, water, qfalse );
		}
		return;
	}

	// if the trace enters water
	if( trace->contents & MASK_WATER )
	{
		vec3_t waterstart;
		vec3_t otherDir;

		if( impactEvent )
			impactEvent( start, trace, qfalse, qtrue );
		VectorCopy( trace->endpos, waterstart );

		// change bullet's course when it enters water
		VectorCopy( spreadDir, otherDir );
		GS_SpreadDir( otherDir, spreadDir, BULLET_WATER_REFRACTION, BULLET_WATER_REFRACTION, seed );
		VectorMA( waterstart, range - DistanceFast( start, waterstart ), spreadDir, end );

		// re-trace ignoring water this time
		contentmask &= ~MASK_WATER;
		GS_Trace( trace, waterstart, vec3_origin, vec3_origin, end, ignore, contentmask, timeDelta );
		if( impactEvent )
			impactEvent( waterstart, trace, qfalse, qfalse );
		return;
	}

	if( impactEvent )
		impactEvent( start, trace, qfalse, qfalse );
}
#undef BULLET_WATER_REFRACTION

//==================================================
// MISC
//==================================================

/*
* GS_FrameForTime
* Returns the frame and interpolation fraction for current time in an animation started at a given time.
* When the animation is finished it will return frame -1. Takes looping into account. Looping animations
* are never finished.
*/
float GS_FrameForTime( int *frame, unsigned int curTime, unsigned int startTimeStamp, float frametime, int firstframe, int lastframe, int loopingframes, qboolean forceLoop )
{
	unsigned int runningtime, framecount;
	int curframe;
	float framefrac;

	if( curTime <= startTimeStamp )
	{
		*frame = firstframe;
		return 0.0f;
	}

	if( firstframe == lastframe )
	{
		*frame = firstframe;
		return 1.0f;
	}

	runningtime = curTime - startTimeStamp;
	framefrac = ( (double)runningtime / (double)frametime );
	framecount = (unsigned int)framefrac;
	framefrac -= framecount;

	curframe = firstframe + framecount;
	if( curframe > lastframe )
	{
		if( forceLoop && !loopingframes )
			loopingframes = lastframe - firstframe;

		if( loopingframes )
		{
			unsigned int numloops;
			unsigned int startcount;

			startcount = ( lastframe - firstframe ) - loopingframes;

			numloops = ( framecount - startcount ) / loopingframes;
			curframe -= loopingframes * numloops;
			if( loopingframes == 1 )
				framefrac = 1.0f;
		}
		else
			curframe = -1;
	}

	*frame = curframe;

	return framefrac;
}

/*
* GS_WaterLevelForBBox
*/
int GS_WaterLevelForBBox( vec3_t origin, vec3_t mins, vec3_t maxs, int *watertype )
{
#define WATER_HEIGHT_FEETS  2
	vec3_t point;
	int waterlevel;
	int contentmask = MASK_PLAYERSOLID;

	// find waterlevel
	waterlevel = 0;
	if( watertype ) *watertype = 0;
	if( contentmask & MASK_SOLID )
	{
		int i, contents;

		// find center of box
		for( i = 0; i < 3; i++ )
			point[i] = origin[i] + maxs[i] + mins[i];

		//  pure center of box is waterlevel 2
		contents = GS_PointContents( point, 0 );
		if( contents & MASK_WATER )
		{
			waterlevel |= WATERLEVEL_FLOAT;
			if( watertype ) *watertype |= ( contents & MASK_WATER );
		}

		// if box height is smaller than STEPSIZE don't make subdivisions.
		if( maxs[2] - mins[2] <= STEPSIZE )
		{
			if( waterlevel )
			{
				waterlevel |= WATERLEVEL_FEETS;
			}
		}
		else
		{
			// check feet water level
			point[2] = origin[2] + mins[2] + ( WATER_HEIGHT_FEETS );
			contents = GS_PointContents( point, 0 );
			if( contents & MASK_WATER )
			{
				waterlevel |= WATERLEVEL_FEETS;
				if( watertype ) *watertype |= ( contents & MASK_WATER );
			}
		}
	}

	return waterlevel;
#undef WATER_HEIGHT_FEETS
}

/*
* GS_TouchPushTrigger
*/
void GS_TouchPushTrigger( int movetype, vec3_t velocity, entity_state_t *pusher )
{
	VectorCopy( pusher->origin2, velocity );
}

/*
* GS_AddTouchEnt
*/
void GS_AddTouchEnt( touchlist_t *touchList, int entNum, cplane_t *plane, int surfaceflags )
{
	int i;

	if( !touchList || touchList->numtouch >= MAXTOUCH || entNum == ENTITY_INVALID )
		return;

	// see if it is already added
	for( i = 0; i < touchList->numtouch; i++ )
	{
		if( touchList->touchents[i] == entNum )
			return;
	}

	// add it
	touchList->touchents[touchList->numtouch] = entNum;
	touchList->touchplanes[touchList->numtouch] = *plane;
	touchList->touchsurfs[touchList->numtouch] = surfaceflags;
	touchList->numtouch++;
}

//==================================================
// VECTOR SNAPPING
//==================================================

/*
* GS_GoodPosition
*/
static qboolean GS_GoodPosition( int snaptorigin[3], vec3_t mins, vec3_t maxs, int passent, int contentmask )
{
	trace_t	trace;
	vec3_t point;
	int i;

	if( !( contentmask & MASK_SOLID ) )
		return qtrue;

	for( i = 0; i < 3; i++ )
		point[i] = (float)snaptorigin[i] * ( 1.0/PM_VECTOR_SNAP );

	GS_Trace( &trace, point, mins, maxs, point, passent, contentmask, 0 );
	return (qboolean)!trace.allsolid;
}

/*
* GS_SnapPosition
*/
qboolean GS_SnapPosition( vec3_t origin, vec3_t mins, vec3_t maxs, int passent, int contentmask )
{
	int sign[3];
	int i, j, bits;
	int base[3];
	int originInt[3];
	// try all single bits first
	static const int jitterbits[8] = { 0, 4, 1, 2, 3, 5, 6, 7 };

	for( i = 0; i < 3; i++ )
	{
		if( origin[i] >= 0 )
			sign[i] = 1;
		else
			sign[i] = -1;
		originInt[i] = (int)( origin[i] * PM_VECTOR_SNAP );
		if( (float)originInt[i] * ( 1.0/PM_VECTOR_SNAP ) == origin[i] )
			sign[i] = 0;
	}

	VectorCopy( originInt, base );

	// try all combinations
	for( j = 0; j < 8; j++ )
	{
		bits = jitterbits[j];
		VectorCopy( base, originInt );
		for( i = 0; i < 3; i++ )
		{
			if( bits & ( 1<<i ) )
				originInt[i] += sign[i];
		}

		if( GS_GoodPosition( originInt, mins, maxs, passent, contentmask ) )
		{
			VectorScale( originInt, ( 1.0/PM_VECTOR_SNAP ), origin );
			return qtrue;
		}
	}

	return qfalse;
}

/*
* GS_SnapInitialPosition
*/
qboolean GS_SnapInitialPosition( vec3_t origin, vec3_t mins, vec3_t maxs, int passent, int contentmask )
{
	int x, y, z;
	int base[3];
	static const int offset[3] = { 0, -1, 1 };
	int originInt[3];

	VectorScale( origin, PM_VECTOR_SNAP, originInt );
	VectorCopy( originInt, base );

	for( z = 0; z < 3; z++ )
	{
		originInt[2] = base[2] + offset[z];
		for( y = 0; y < 3; y++ )
		{
			originInt[1] = base[1] + offset[y];
			for( x = 0; x < 3; x++ )
			{
				originInt[0] = base[0] + offset[x];
				if( GS_GoodPosition( originInt, mins, maxs, passent, contentmask ) )
				{
					origin[0] = originInt[0]*( 1.0/PM_VECTOR_SNAP );
					origin[1] = originInt[1]*( 1.0/PM_VECTOR_SNAP );
					origin[2] = originInt[2]*( 1.0/PM_VECTOR_SNAP );
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/*
* GS_SnapVelocity
*/
void GS_SnapVector( vec3_t velocity )
{
	int i, velocityInt[3];
	// snap velocity to sixteenths
	for( i = 0; i < 3; i++ )
	{
		velocityInt[i] = (int)( velocity[i] * PM_VECTOR_SNAP );
		velocity[i] = (float)velocityInt[i] * ( 1.0/PM_VECTOR_SNAP );
	}
}

/*
* GS_ClampPlayerAngles
*/
void GS_ClampPlayerAngles( short cmd_angles[3], short delta_angles[3], float angles[3] )
{
	int i;
	short temp;

	for( i = 0; i < 3; i++ )
	{
		temp = cmd_angles[i] + delta_angles[i];
		if( i == PITCH )
		{           // don't let the player look up or down more than 90 degrees
			if( temp > 16000 )
			{
				delta_angles[i] = 16000 - cmd_angles[i];
				temp = 16000;
			}
			else if( temp < -16000 )
			{
				delta_angles[i] = -16000 - cmd_angles[i];
				temp = -16000;
			}
		}

		angles[i] = SHORT2ANGLE( temp );
	}
}

#define MOVE_SLIDE_CLAMPING

/*
* GS_BounceVelocity
*/
void GS_BounceVelocity( vec3_t velocity, vec3_t normal, vec3_t newvelocity, float overbounce )
{
	float backoff;
	float change;
	int i;

	backoff = DotProduct( velocity, normal );
	if( backoff < 0 )
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for( i = 0; i < 3; i++ )
	{
		change = normal[i] * backoff;
		newvelocity[i] = velocity[i] - change;
	}
#ifdef MOVE_SLIDE_CLAMPING
	{
		float oldspeed, newspeed;
		oldspeed = VectorLength( velocity );
		newspeed = VectorLength( newvelocity );
		if( newspeed > oldspeed )
		{
			VectorNormalize( newvelocity );
			VectorScale( newvelocity, oldspeed, newvelocity );
		}
	}
#endif
}

/*
* GS_ClipVelocity
*/
void GS_ClipVelocity( vec3_t velocity, vec3_t normal, vec3_t newvelocity )
{
	float backoff;
	int i;

	backoff = DotProduct( velocity, normal );
	for( i = 0; i < 3; i++ )
	{
		newvelocity[i] = velocity[i] - ( normal[i] * backoff );
	}

#ifdef MOVE_SLIDE_CLAMPING
	{
		float oldspeed, newspeed;
		oldspeed = VectorLength( velocity );
		newspeed = VectorLength( newvelocity );
		if( newspeed > oldspeed )
		{
			VectorNormalize( newvelocity );
			VectorScale( newvelocity, oldspeed, newvelocity );
		}
	}
#endif
}
