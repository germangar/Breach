/*
 */

#include "g_local.h"

//
// g_clip.c - entity contact detection
//

extern cvar_t *g_antilag;
extern cvar_t *g_antilag_maxtimedelta;

#define	CFRAME_UPDATE_BACKUP	64  // copies of entity_state_t to keep buffered (1 second of backup at 62 fps).
#define	CFRAME_UPDATE_MASK	( CFRAME_UPDATE_BACKUP-1 )

typedef struct c4frame_s
{
	entity_state_t clipStates[MAX_EDICTS];
	int numedicts;

	unsigned int timestamp;
	unsigned int framenum;
} c4frame_t;

c4frame_t sv_collisionframes[CFRAME_UPDATE_BACKUP];
static unsigned int sv_collisionFrameNum = 0;

/*
* 
*/
void GClip_BackUpCollisionFrame( void )
{
	c4frame_t *cframe;
	int i;

	if( !g_antilag->integer )
		return;

	// fixme: should check for any validation here?

	cframe = &sv_collisionframes[sv_collisionFrameNum & CFRAME_UPDATE_MASK];
	cframe->timestamp = game.serverTime;
	cframe->framenum = sv_collisionFrameNum;
	sv_collisionFrameNum++;

	//backup edicts
	for( i = 0; i < game.numentities; i++ )
	{
		cframe->clipStates[i] = game.entities[i].s;
	}
	cframe->numedicts = game.numentities;
}

/*
* 
*/
entity_state_t *GClip_GetClipStateForDeltaTime( int entNum, int deltaTime )
{

	static entity_state_t clipState;
	static entity_state_t clipStateNewer; // for interpolation
	c4frame_t *cframe = NULL;
	unsigned int backTime, cframenum, backframes, i;
	gentity_t *ent = game.entities + entNum;

	if( entNum == ENTITY_WORLD || deltaTime >= 0 || !g_antilag->integer )
	{                                                                   // current time entity
		return &ent->s;
	}

	// see if this entity was freed inside the timeDelta threshold (and is not currently in use)
	if( ( ent->freetimestamp < trap_Milliseconds() + deltaTime ) && !ent->s.local.inuse )
		return NULL;

	// clamp delta time inside the backed up limits
	backTime = abs( deltaTime );
	if( g_antilag_maxtimedelta->integer )
	{
		if( g_antilag_maxtimedelta->integer < 0 )
			trap_Cvar_SetValue( "g_antilag_maxtimedelta", abs( g_antilag_maxtimedelta->integer ) );
		if( backTime > (unsigned int)g_antilag_maxtimedelta->integer )
			backTime = (unsigned int)g_antilag_maxtimedelta->integer;
	}

	// find the first snap with timestamp < than servertime - backtime
	cframenum = sv_collisionFrameNum;
	for( backframes = 1; backframes < CFRAME_UPDATE_BACKUP && backframes < sv_collisionFrameNum; backframes++ ) // never overpass limits
	{
		cframe = &sv_collisionframes[( cframenum-backframes ) & CFRAME_UPDATE_MASK];
		// if solid has changed, we can't keep moving backwards
		if( ent->s.solid != cframe->clipStates[entNum].solid || ent->s.local.inuse != cframe->clipStates[entNum].local.inuse )
		{
			backframes--;
			if( backframes == 0 )
			{           // we can't step back from first
				cframe = NULL;
			}
			else
			{
				cframe = &sv_collisionframes[( cframenum-backframes ) & CFRAME_UPDATE_MASK];
			}
			break;
		}

		if( game.serverTime >= cframe->timestamp + backTime )
			break;
	}

	if( !cframe )
	{           // current time entity
		return &ent->s;
	}

	// setup with older for the data that is not interpolated
	clipState = cframe->clipStates[entNum];

	// if we found an older than desired backtime frame, interpolate to find a more precise position.
	if( game.serverTime > cframe->timestamp + backTime )
	{
		float lerpFrac;

		if( backframes == 1 )
		{               // interpolate from 1st backed up to current
			lerpFrac = (float)( ( game.serverTime - backTime ) - cframe->timestamp ) / (float)( game.serverTime - cframe->timestamp );
			clipStateNewer = ent->s;
		}
		else
		{ // interpolate between 2 backed up
			c4frame_t *cframeNewer = &sv_collisionframes[( cframenum-( backframes-1 ) ) & CFRAME_UPDATE_MASK];
			lerpFrac = (float)( ( game.serverTime - backTime ) - cframe->timestamp ) / (float)( cframeNewer->timestamp - cframe->timestamp );
			clipStateNewer = cframeNewer->clipStates[entNum];
		}

		//GS_Printf( "backTime:%i cframeBackTime:%i backFrames:%i lerfrac:%f\n", backTime, game.serverTime - cframe->timestamp, backframes, lerpFrac );

		// interpolate
		VectorLerp( clipState.ms.origin, lerpFrac, clipStateNewer.ms.origin, clipState.ms.origin );
		VectorLerp( clipState.local.mins, lerpFrac, clipStateNewer.local.mins, clipState.local.mins );
		VectorLerp( clipState.local.maxs, lerpFrac, clipStateNewer.local.maxs, clipState.local.maxs );
		for( i = 0; i < 3; i++ )
			clipState.ms.angles[i] = LerpAngle( clipState.ms.angles[i], clipStateNewer.ms.angles[i], lerpFrac );
	}

	//GS_Printf( "backTime:%i cframeBackTime:%i backFrames:%i\n", backTime, game.serverTime - cframe->timestamp, backframes );

	// back time entity
	return &clipState;
}

/*
* GClip_PointCluster
* we don't have a trap for CM_PointLeafnum, and I don't think it's worth adding it
*/
int GClip_PointCluster( vec3_t origin )
{
	int num_leafs;
	int leafs[1];

	num_leafs = trap_CM_BoxLeafnums( origin, origin, leafs, 1, NULL );
	return trap_CM_LeafCluster( leafs[0] );
}

/*
* G_Clip_AreaForPoint
* we don't have a trap for CM_PointLeafnum, and I don't think it's worth adding it
*/
// define QUICK_AREAFORPOINT
int G_Clip_AreaForPoint( vec3_t origin )
{
#ifdef QUICK_AREAFORPOINT
	int leafs[1], numLeafs;
	numLeafs = trap_CM_BoxLeafnums( origin, origin, leafs, 1, NULL );
	if( !numLeafs )
		return -1;

	return trap_CM_LeafArea( leafs[0] );
#else
	int i, numLeafs;
	int areas[4], area;
	int leafs[4];

	// a point shouldn't return more than one leaf, so 4 leafs should be useless
	// but I prefer having it by now so the code warns me if it ever happens

	numLeafs = trap_CM_BoxLeafnums( origin, origin, leafs, 4, NULL );
	if( numLeafs > 1 )
		GS_Printf( "G_Clip_AreaForPoint : numLeafs %i\n", numLeafs );

	area = -1;
	for( i = numLeafs - 1; i >= 0; i-- )
	{
		areas[i] = trap_CM_LeafArea( leafs[i] );

		if( areas[i] != -1 && area != -1 && area != areas[i] )
			GS_Printf( "G_Clip_AreaForPoint : more than one valid area returned\n" );

		if( areas[i] != -1 )
			area = areas[i];
	}

	if( area == -1 )
		GS_Printf( "G_Clip_AreaForPoint : No valid area returned\n" );

	return area;
#endif
}

#define MAX_TOTAL_ENT_LEAFS	128

/*
* GClip_SetEntityAreaInfo
*/
void GClip_SetEntityAreaInfo( gentity_t *ent )
{
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int clusters[MAX_TOTAL_ENT_LEAFS];
	int num_leafs;
	int i, j;
	int leafarea;
	int topnode;

	if( !ent->s.local.inuse || ent->s.number == ENTITY_WORLD )
		return;

	// link to PVS leafs
	ent->vis.num_clusters = 0;
	ent->vis.areanum = -1;
	ent->vis.areanum2 = -1;

	GS_AbsoluteBoundsForEntity( &ent->s );

	// get all leafs, including solids
	num_leafs = trap_CM_BoxLeafnums( ent->s.local.absmins, ent->s.local.absmaxs,
	                                 leafs, MAX_TOTAL_ENT_LEAFS, &topnode );

	// set areas
	for( i = 0; i < num_leafs; i++ )
	{
		clusters[i] = trap_CM_LeafCluster( leafs[i] );
		leafarea = trap_CM_LeafArea( leafs[i] );
		if( leafarea > -1 )
		{
			// doors may legally straggle two areas,
			// but nothing should ever need more than that
			if( ent->vis.areanum > -1 && ent->vis.areanum != leafarea )
			{
				if( ent->vis.areanum2 > -1 && ent->vis.areanum2 != leafarea )
				{
					if( developer->integer ) 
						GS_Printf( "Object touching 3 areas at %f %f %f\n", ent->s.local.absmins[0], ent->s.local.absmins[1], ent->s.local.absmins[2] );
				}
				ent->vis.areanum2 = leafarea;
			}
			else
				ent->vis.areanum = leafarea;
		}
	}

	if( num_leafs >= MAX_TOTAL_ENT_LEAFS ) // assume we missed some leafs, and mark by headnode
	{
		ent->vis.num_clusters = -1;
		ent->vis.headnode = topnode;
	}
	else
	{
		ent->vis.num_clusters = 0;
		for( i = 0; i < num_leafs; i++ )
		{
			if( clusters[i] == -1 )
				continue; // not a visible leaf
			for( j = 0; j < i; j++ )
				if( clusters[j] == clusters[i] )
					break;
			if( j == i )
			{
				if( ent->vis.num_clusters == MAX_ENT_CLUSTERS ) // assume we missed some leafs, and mark by headnode
				{
					ent->vis.num_clusters = -1;
					ent->vis.headnode = topnode;
					break;
				}

				ent->vis.clusternums[ent->vis.num_clusters++] = clusters[i];
			}
		}
	}

	// if the entity is a door update it's areaportal state

	// entity must touch at least two areas
	if( ent->mover.is_areaportal && ent->vis.areanum > -1 && ent->vis.areanum2 > -1 )
	{
		if( ent->last_door_portal_state != ent->door_portal_state )
		{
			trap_CM_SetAreaPortalState( ent->vis.areanum, ent->vis.areanum2, ent->door_portal_state );
			ent->last_door_portal_state = ent->door_portal_state;
		}
	}
}

/*
* GClip_SetBrushModel
*
* Also sets mins and maxs for inline bmodels
*/
void GClip_SetBrushModel( gentity_t *ent, char *name )
{
	struct cmodel_s *cmodel;

	if( !name )
		GS_Error( "PF_setmodel: NULL" );

	if( !name[0] )
	{
		ent->s.modelindex1 = 0;
		return;
	}

	if( name[0] != '*' )
	{
		ent->s.type = ET_MODEL;
		ent->s.modelindex1 = trap_ModelIndex( name );
		//trap_PureModel( name );
		return;
	}

	// if it is an inline model, get the size information for it
	ent->s.cmodeltype = CMODEL_BRUSH;

	if( !strcmp( name, "*0" ) )
	{
		ent->s.modelindex1 = 0;
		cmodel = trap_CM_InlineModel( 0 );
		trap_CM_InlineModelBounds( cmodel, ent->s.local.mins, ent->s.local.maxs );
		trap_PureModel( name );
		return;
	}

	ent->s.type = ET_MODEL;
	ent->s.modelindex1 = trap_ModelIndex( name );
	assert( ent->s.modelindex1 == atoi( name + 1 ) );
	cmodel = trap_CM_InlineModel( ent->s.modelindex1 );
	trap_CM_InlineModelBounds( cmodel, ent->s.local.mins, ent->s.local.maxs );
	trap_PureModel( name );
	GClip_LinkEntity( ent );
}

/*
* 
*/
void GClip_SetEntityBounds( gentity_t *ent )
{
	struct cmodel_s *cmodel;
	int i;

	if( !ent->s.local.inuse )
		return;

	ent->s.bbox = 0;

	// world entity is special
	if( ent == worldEntity )
	{
		if( ent->s.cmodeltype != CMODEL_BRUSH )
		{
			GS_Printf( "WARNING: GClip_SetEntityBounds: Fixing worldEntity without CMODEL_BRUSH set\n" );
			ent->s.cmodeltype = CMODEL_BRUSH;
		}
		if( ent->s.modelindex1 != 1 )
		{
			GS_Printf( "WARNING: GClip_SetEntityBounds: Fixing worldEntity with modelindex1 != 1\n" );
			ent->s.modelindex1 = 1;
		}

		cmodel = trap_CM_InlineModel( 0 );
		trap_CM_InlineModelBounds( cmodel, ent->s.local.mins, ent->s.local.maxs );

		// the world is never added to the space partition, so update absolute bounds here.
		for( i = 0; i < 3; i++ )
		{
			ent->s.local.boundmins[i] = ent->s.local.absmins[i] = ent->s.local.mins[i];
			ent->s.local.boundmaxs[i] = ent->s.local.absmaxs[i] = ent->s.local.maxs[i];
		}
	}
	// brush models don't use custom boxes
	else if( ent->s.cmodeltype == CMODEL_BRUSH )
	{
		cmodel = trap_CM_InlineModel( ent->s.modelindex1 );
		trap_CM_InlineModelBounds( cmodel, ent->s.local.mins, ent->s.local.maxs );
	}
	// bboxes are encoded into a short for net transmission
	else if( ent->s.cmodeltype == CMODEL_BBOX || ent->s.cmodeltype == CMODEL_BBOX_ROTATED )
	{
		ent->s.bbox = GS_EncodeEntityBBox( ent->s.local.mins, ent->s.local.maxs );
	}
}

/*
* GClip_UnlinkEntity
* call before removing an entity, and before trying to move one,
* so it doesn't clip against itself
*/
void GClip_UnlinkEntity( gentity_t *ent )
{
	GS_SpaceUnLinkEntity( &ent->s );

	// remove the activable bit
	ent->s.flags &= ~SFL_ACTIVABLE;
}

/*
* GClip_LinkEntity
* Needs to be called any time an entity changes origin, mins, maxs, or solid.
*/
void GClip_LinkEntity( gentity_t *ent )
{
	qboolean linked;

	GClip_UnlinkEntity( ent );

	if( !ent->s.local.inuse )
		return;

	GClip_SetEntityBounds( ent );

	if( ent->s.number == ENTITY_WORLD )
		return;

	linked = GS_SpaceLinkEntity( &ent->s );
	if( linked )
	{
		// set the activable bit for entities that can be activated
		if( ent->s.type == ET_ITEM )
		{
			gsitem_t *item;
			item = GS_FindItemByIndex( ent->s.modelindex1 );
			if( item && ( item->flags & ITFLAG_PICKABLE ) )
			{
				ent->s.flags |= SFL_ACTIVABLE;
			}
		}
		else if( ( ent->s.solid != SOLID_TRIGGER ) && ent->activate && !ent->targetname )
			ent->s.flags |= SFL_ACTIVABLE;
	}
}

//===========================================================================

/*
* GClip_TouchTriggers
*/
void GClip_TouchTriggers( gentity_t *ent )
{
	int i, j;
	gentity_t *touch;
	trace_t	trace;
	int contentmask = CONTENTS_TRIGGER;
	static gs_traceentitieslist clipList;

	if( ent->s.solid == SOLID_NOT || ent->s.solid == SOLID_CORPSE )
		return;

	if( ent->s.solid == SOLID_PROJECTILE )
		return;

	GS_OctreeCreatePotentialCollideList( &clipList, ent->s.ms.origin, ent->s.local.mins, ent->s.local.maxs, ent->s.ms.origin );
	if( !clipList.numEntities )
		return;

	for( j = 0; j < clipList.numEntities; j++ )
	{
		i = clipList.entityNums[j];
		if( i == ENTITY_WORLD )
			continue;

		touch = &game.entities[i];
		if( !touch->s.local.inuse )
			continue;
		// see if it's any trigger
		if( touch->s.solid != SOLID_TRIGGER )
			continue;

		GS_TraceToEntity( &trace, ent->s.ms.origin, ent->s.local.mins, ent->s.local.maxs, ent->s.ms.origin, ent->s.number, contentmask, &touch->s );
		if( trace.startsolid || trace.allsolid )
		{
			if( touch->touch )
				touch->touch( touch, ent, NULL, 0 );
		}
	}
}

