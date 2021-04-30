/*
   Copyright (C) 2007 German Garcia
 */

#include "gs_local.h"

//==================================================
//	SPACE PARTITION
//==================================================

#define OCNODE_MINIMUM_RADIUS	256

#define OCNODE_RIGHT	1
#define OCNODE_TOP	1
#define OCNODE_FRONT	1

#define OCNODE_LEFT -1
#define OCNODE_BOTTOM	-1
#define OCNODE_BACK -1

#define OCNODE_MAXCHILDREN  8

static vec_t octDirs[OCNODE_MAXCHILDREN][3] = {
	{ OCNODE_LEFT, OCNODE_TOP, OCNODE_FRONT },
	{ OCNODE_LEFT, OCNODE_TOP, OCNODE_BACK },
	{ OCNODE_RIGHT, OCNODE_TOP, OCNODE_BACK },
	{ OCNODE_RIGHT, OCNODE_TOP, OCNODE_FRONT },
	{ OCNODE_LEFT, OCNODE_BOTTOM, OCNODE_FRONT },
	{ OCNODE_LEFT, OCNODE_BOTTOM, OCNODE_BACK },
	{ OCNODE_RIGHT, OCNODE_BOTTOM, OCNODE_BACK },
	{ OCNODE_RIGHT, OCNODE_BOTTOM, OCNODE_FRONT }
};

typedef struct octree_node_s
{

	float radius;
	vec3_t origin, absmins, absmaxs;

	struct octree_node_s *children[OCNODE_MAXCHILDREN];
	struct octree_node_s *parent;

	int level;
	int numChildren;

	qboolean sublinks;
	int numLinkedStates;
	struct entity_state_s *linkedStates[128];
} octree_node_t;

static octree_node_t *ocRoot = NULL;

static unsigned int maxlevel = 0;
static unsigned int numnodes = 0;
static unsigned int traceCount = 0;

//==================================================
//	LINKING
//==================================================
#define MAX_ENTITY_LINKS 8
typedef struct
{
	int numlinks;
	struct octree_node_s *nodes[MAX_ENTITY_LINKS];
	unsigned int traceNum;
} gs_entity_links_t;

static gs_entity_links_t entitiesLinks[MAX_EDICTS];
static unsigned int numLinkedEntities = 0;

// generated list of entities to trace against
static gs_traceentitieslist trace_list;

// set multiple ignores for the next trace
#define MAX_IGNORES 16
static int ignoreList[MAX_IGNORES];
static int numIgnores;

//=================
//GS_PushTraceCount
//=================
static void GS_PushTraceCount( void )
{
	int i;
	if( traceCount >= 0x70000000 )
	{
		traceCount = 0;
		for( i = 0; i < MAX_EDICTS; i++ )
			entitiesLinks[i].traceNum = 0;
	}
	traceCount++;
}

//=================
//GS_ClearAllEntityLinks
//=================
static void GS_ClearAllEntityLinks( void )
{
	memset( entitiesLinks, 0, sizeof( entitiesLinks ) );
	traceCount = 0;
	numLinkedEntities = 0;
	numIgnores = 0;
}

//=================
//GS_SpaceUnLinkEntity
//=================
qboolean GS_SpaceUnLinkEntity( entity_state_t *state )
{
	int i, j, k;
	entity_state_t *st;
	struct octree_node_s *node;

	if( !entitiesLinks[state->number].numlinks )
		return qfalse;

	for( i = 0; i < entitiesLinks[state->number].numlinks; i++ )
	{
		node = entitiesLinks[state->number].nodes[i];
		assert( node && node->numLinkedStates );

		for( j = 0; j < node->numLinkedStates; j++ )
		{
			assert( node->linkedStates[j] );
			st = node->linkedStates[j];
			if( st->number == state->number )
				break;
		}

		if( j == node->numLinkedStates )
			GS_Error( "GS_SpaceUnLinkEntity: bad node referenced\n" );

		for( k = j; k < node->numLinkedStates; k++ )
		{
			node->linkedStates[k] = node->linkedStates[k+1];
		}
		node->linkedStates[node->numLinkedStates] = NULL;
		node->numLinkedStates--;

		// if this node has no links anymore, navigate the tree upwards and
		// see if the parents have no sublinks, and update accordingly
		if( !node->numLinkedStates )
		{
			octree_node_t *parent = node->parent;
			qboolean sublinkfound;

			node->sublinks = qfalse; // mark self as not having sublinks
			while( parent != NULL )
			{
				sublinkfound = qfalse;
				for( j = 0; j < OCNODE_MAXCHILDREN; j++ )
				{
					if( parent->children[j] && parent->children[j]->sublinks )
					{
						sublinkfound = qtrue;
						break;
					}
				}

				if( sublinkfound )  // no need to continue
					break;

				parent->sublinks = qfalse;
				parent = parent->parent;
			}
		}
	}

	entitiesLinks[state->number].numlinks = 0;
	numLinkedEntities--;

	return qtrue;
}

//=================
//GS_RecurseLinkEntityToOctreeNode
//=================
static int GS_RecurseLinkEntityToOctreeNode( entity_state_t *state, octree_node_t *node, int entityRadius )
{
	int i;
	qboolean canRecurse = qtrue;

	if( !BoundsIntersect( state->local.absmins, state->local.absmaxs, node->absmins, node->absmaxs ) )
		return 0;

	// we don't want to go down if the entity box is bigger than the next level nodes boxes
	if( entityRadius + 1 > node->radius * 0.5f )
		canRecurse = qfalse;

	if( canRecurse )
	{
		int linked = 0;
		for( i = 0; i < OCNODE_MAXCHILDREN; i++ )
		{
			if( node->children[i] )
				linked += GS_RecurseLinkEntityToOctreeNode( state, node->children[i], entityRadius );
		}

		// if it was linked at a lower level, don't link it here
		if( linked )
			return linked;
	}

	//assert( entitiesLinks[state->number].numlinks < MAX_ENTITY_LINKS );
	if( entitiesLinks[state->number].numlinks == MAX_ENTITY_LINKS )
		GS_Error( "GS_RecurseLinkEntityToOctreeNode: MAX_ENTITY_LINKS reached\n" );

	// link the entity on the node
	if( node->numLinkedStates + 1 == 128 )
		GS_Error( "GS_RecurseLinkEntityToOctreeNode: node->numLinkedStates + 1 == 128\n" );

	node->linkedStates[node->numLinkedStates] = state;
	node->numLinkedStates++;

	// keep track of the node
	entitiesLinks[state->number].nodes[entitiesLinks[state->number].numlinks] = node;
	entitiesLinks[state->number].numlinks++;

	// navigate the tree upwards and mark all parents as having entities linked at lower level
	{
		octree_node_t *parent = node->parent;
		node->sublinks = qtrue; // mark self as having sublinks too
		while( parent != NULL )
		{
			parent->sublinks = qtrue;
			parent = parent->parent;
		}
	}

	return 1;
}

//=================
//GS_SpaceLinkEntity
//=================
qboolean GS_SpaceLinkEntity( entity_state_t *state )
{
	vec3_t size;
	float radius = 0;
	int i;

	GS_SpaceUnLinkEntity( state );

	if( state->solid == SOLID_NOT || state->cmodeltype == CMODEL_NOT )
		return qfalse;

	if( state->number == ENTITY_WORLD )
		return qfalse;

	// update absmins and absmaxs
	GS_AbsoluteBoundsForEntity( state );

	// find out the entity inner radius. We don't want to link it to nodes smaller than the entity
	VectorSubtract( state->local.maxs, state->local.mins, size );
	VectorScale( size, 0.5, size );
	for( radius = 0, i = 0; i < 3; i++ )
	{
		if( size[i] > radius )
			radius = size[i];
	}

	if( ocRoot && GS_RecurseLinkEntityToOctreeNode( state, ocRoot, radius ) )
	{
		numLinkedEntities++;
		return qtrue;
	}

	return qfalse;
}

//==================================================
//	OCTREE
//==================================================

// it draws it until it runs out of polygons. Don't expect the whole thing in big maps
static void GS_RecurseDrawOctreeNode( octree_node_t *node )
{
	int i;

	if( !node || !node->sublinks )
		return;

	if( node->numLinkedStates )
		module.DrawBox( vec3_origin, node->absmins, node->absmaxs, vec3_origin, NULL );

	for( i = 0; i < OCNODE_MAXCHILDREN; i++ )
	{
		if( node->children[i] )
			GS_RecurseDrawOctreeNode( node->children[i] );
	}
}

//=================
//GS_DrawSpacePartition
//=================
void GS_DrawSpacePartition( void )
{
	if( module.type != GS_MODULE_CGAME )
		return;

	GS_RecurseDrawOctreeNode( ocRoot );
}

//=================
//GS_CubeIntersectsWorld
//=================
static inline qboolean GS_CubeIntersectsWorld( vec3_t origin, float radius )
{
#define CHECKLEAFCOUNT	8
#define EXTRASIZE	0.1f
	vec3_t absmins, absmaxs;
	int i, cluster, num_leafs, leafs[CHECKLEAFCOUNT];

	for( i = 0; i < 3; i++ )
	{
		absmins[i] = origin[i] - ( radius + EXTRASIZE );
		absmaxs[i] = origin[i] + ( radius + EXTRASIZE );
	}

	// check if valid cluster
	cluster = -1;
	num_leafs = module.trap_CM_BoxLeafnums( absmins, absmaxs, leafs, CHECKLEAFCOUNT, NULL );
	for( i = 0; i < num_leafs; i++ )
	{
		cluster = module.trap_CM_LeafCluster( leafs[i] );
		if( cluster != -1 )
			break;
	}

	if( cluster == -1 )
		return qfalse;

#if 1
	{
		trace_t trace;
		// check for having any solid part of the world inside the node (note: this doesn't include brush models)
		module.trap_CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, absmins, absmaxs, NULL, MASK_PLAYERSOLID, NULL, NULL );
		if( !trace.startsolid )
		{
			return qfalse;
		}
	}
#endif

	return qtrue;

#undef CHECKLEAFCOUNT
#undef EXTRASIZE
}

//=================
//GS_RecurseCreateOctreeNode
//=================
static octree_node_t *GS_RecurseCreateOctreeNode( vec3_t origin, float radius, octree_node_t *parent, unsigned int level )
{
	octree_node_t *node;
	int i, j;

	// see if this node is worth adding
	if( level && radius < OCNODE_MINIMUM_RADIUS )
		return NULL;

	if( !GS_CubeIntersectsWorld( origin, radius ) )
		return NULL;

	node = ( octree_node_t * )module_Malloc( sizeof( octree_node_t ) );
	VectorCopy( origin, node->origin );
	for( i = 0; i < 3; i++ )
	{
		node->absmins[i] = node->origin[i] - radius;
		node->absmaxs[i] = node->origin[i] + radius;
	}
	node->radius = radius;
	node->parent = parent;
	node->level = level;
	node->numChildren = 0;
	node->numLinkedStates = 0;
	node->sublinks = qfalse;

	// stats
	numnodes++;
	if( level > maxlevel )
		maxlevel = level;

	for( i = 0; i < OCNODE_MAXCHILDREN; i++ )
	{
		vec3_t childOrigin;
		for( j = 0; j < 3; j++ )
		{
			childOrigin[j] = node->origin[j] + ( octDirs[i][j] * ( node->radius * 0.5 ) );
		}
		node->children[i] = GS_RecurseCreateOctreeNode( childOrigin, node->radius * 0.5, node, level + 1 );
		if( node->children[i] != NULL )
			node->numChildren++;
	}

	return node;
}

//=================
//GS_RecurseFreeNode
//=================
static octree_node_t *GS_RecurseFreeNode( octree_node_t *node )
{
	int i;

	for( i = 0; i < OCNODE_MAXCHILDREN; i++ )
	{
		if( node->children[i] )
			node->children[i] = GS_RecurseFreeNode( node->children[i] );
	}

	module_Free( node );
	return NULL;
}

//=================
//GS_FreeSpacePartition
//=================
void GS_FreeSpacePartition( void )
{
	GS_ClearAllEntityLinks();
	if( ocRoot )
	{
		ocRoot = GS_RecurseFreeNode( ocRoot );
		maxlevel = 0;
		numnodes = 0;
	}
}

//=================
//GS_CreateSpacePartition
//=================
void GS_CreateSpacePartition( void )
{
	struct cmodel_s *cmodel;
	vec3_t world_mins, world_maxs;
	vec3_t origin, size;
	float radius;
	int i;

	GS_FreeSpacePartition();

	// find center of the world
	cmodel = module.trap_CM_InlineModel( 0 );
	module.trap_CM_InlineModelBounds( cmodel, world_mins, world_maxs );

	VectorSubtract( world_maxs, world_mins, size );
	VectorScale( size, 0.5, size );
	VectorAdd( world_mins, size, origin );
	for( radius = 0, i = 0; i < 3; i++ )
	{
		if( size[i] > radius )
			radius = size[i];
	}

	ocRoot = GS_RecurseCreateOctreeNode( origin, radius, NULL, 0 );
	if( !ocRoot )
		GS_Error( "GS_CreateSpacePartition: Bad world bounds\n" );

	GS_Printf( "Created OcTree space partition with %i nodes in %i levels\n", numnodes, maxlevel );
}

//=================
//GS_AddTraceIgnore
//=================
void GS_AddTraceIgnore( int entNum )
{
	if( entNum < 0 || entNum >= MAX_EDICTS )
		return;

	if( numIgnores )
	{
		int i;
		for( i = 0; i < numIgnores; i++ )
		{
			if( ignoreList[i] == entNum )
				return;
		}
	}

	ignoreList[numIgnores] = entNum;
	numIgnores++;
	if( numIgnores == MAX_IGNORES )
		GS_Error( "GS_AddTraceIgnore: numIgnores == MAX_IGNORES\n" );
}

//=================
//GS_ClearTraceIgnores
//=================
void GS_ClearTraceIgnores( void )
{
	numIgnores = 0;
}

//=================
//GS_OctreeAddEntityToTraceList
//=================
static inline void GS_OctreeAddEntityToTraceList( gs_traceentitieslist *traceList, int entNum )
{
	if( entitiesLinks[entNum].traceNum == traceCount )  // already added
		return;

	entitiesLinks[entNum].traceNum = traceCount; // mark as added for this trace

	if( numIgnores )
	{
		int i;
		for( i = 0; i < numIgnores; i++ )
		{
			if( ignoreList[i] == entNum )
				return;
		}
	}

	traceList->entityNums[traceList->numEntities] = entNum;
	traceList->numEntities++;
}

//=================
//GS_OctreeRecurseAddLinkedEntities -  Traverse tree and get linked entities
//=================
static void GS_OctreeRecurseAddLinkedEntities( gs_traceentitieslist *traceList, octree_node_t *node, vec3_t tracemins, vec3_t tracemaxs )
{
	int i;

	// there's nothing linked following this thread
	if( !node->sublinks )
		return;

	// if the node doesn't touch the trace bounds, no need to continue this thread
	if( !BoundsIntersect( tracemins, tracemaxs, node->absmins, node->absmaxs ) )
		return;

	if( node->numChildren )
	{
		for( i = 0; i < OCNODE_MAXCHILDREN; i++ )
		{
			if( node->children[i] )
				GS_OctreeRecurseAddLinkedEntities( traceList, node->children[i], tracemins, tracemaxs );
		}
	}

	// ok, let's add the linked entities inside this node
	if( node->numLinkedStates )
	{
		for( i = 0; i < node->numLinkedStates && node->linkedStates[i]; i++ )
		{
			GS_OctreeAddEntityToTraceList( traceList, node->linkedStates[i]->number );
		}
	}
}

//=================
//GS_OctreeCreatePotentialCollideList
//=================
void GS_OctreeCreatePotentialCollideList( gs_traceentitieslist *traceList, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end )
{
	int i;
	vec3_t tracemins, tracemaxs;

	traceList->numEntities = 0;
	GS_PushTraceCount();

	if( !ocRoot )
		return;

	for( i = 0; i < 3; i++ )
	{
		if( end[i] > start[i] )
		{
			tracemins[i] = start[i] + mins[i] - 1;
			tracemaxs[i] = end[i] + maxs[i] + 1;
		}
		else
		{
			tracemins[i] = end[i] + mins[i] - 1;
			tracemaxs[i] = start[i] + maxs[i] + 1;
		}
	}

	GS_OctreeRecurseAddLinkedEntities( traceList, ocRoot, tracemins, tracemaxs );
}

#undef MAX_IGNORES

//==================================================
//	TRACING
//==================================================

//=================
//GS_ContentsForSolid
//=================
static inline int GS_ContentsForSolid( int solid )
{
	switch( solid )
	{
	case SOLID_NOT:
	default:
		break;
	case SOLID_SOLID: // returns all excepting trigger and lets the trace make it's own decision
		return ( CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME );
	case SOLID_PLAYER:
		return CONTENTS_BODY;
	case SOLID_CORPSE:
		return CONTENTS_CORPSE;
	case SOLID_PROJECTILE:
		return CONTENTS_PROJECTILE;
	case SOLID_TRIGGER:
		return CONTENTS_TRIGGER;
	}

	return 0;
}

//=================
//GS_ContentMaskForSolid
//=================
int GS_ContentMaskForSolid( int solid )
{
	return GS_ContentsForSolid( solid );
}

//=================
//GS_PointContents
//=================
int GS_PointContents( vec3_t point, int timeDelta )
{
	int i, j;
	int contents, c2;
	struct cmodel_s	*cmodel;
	float *angles;
	entity_state_t *clipState;

	contents = module.trap_CM_TransformedPointContents( point, NULL, NULL, NULL );

	// ask the space partition for a list of entities to test against
	GS_OctreeCreatePotentialCollideList( &trace_list, point, vec3_origin, vec3_origin, point );

	for( j = 0; j < trace_list.numEntities; j++ )
	{
		i = trace_list.entityNums[j];
		if( i == ENTITY_WORLD )
			continue;

		clipState = module.GetClipStateForDeltaTime( i, timeDelta );
		if( !clipState )
			continue;

		// might intersect, so do an exact clip
		cmodel = GS_CModelForEntity( clipState );
		if( !cmodel )
			continue;

		if( clipState->cmodeltype == CMODEL_BRUSH || clipState->cmodeltype == CMODEL_BBOX_ROTATED )
			angles = clipState->ms.angles;
		else
			angles = vec3_origin;

		c2 = module.trap_CM_TransformedPointContents( point, cmodel, clipState->ms.origin, angles );
		contents |= c2;
	}

	return contents;
}

//==================
//GS_TraceToEntity - Trace a box against a given entity state
//==================
void GS_TraceToEntity( trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask, entity_state_t *state )
{
	struct cmodel_s	*cmodel;
	float *angles;

	if( state->number != passent )
	{
		if( ( GS_ContentsForSolid( state->solid ) & contentmask ) || ( contentmask == MASK_ALL ) )
		{
			if( ( cmodel = GS_CModelForEntity( state ) ) != NULL )
			{
				if( state->cmodeltype == CMODEL_BRUSH || state->cmodeltype == CMODEL_BBOX_ROTATED )
					angles = state->ms.angles;
				else
					angles = vec3_origin;

				// fixme? : tracing with CONTENTS_TRIGGER as mask ignores trigger
				// boxes and brushes. It needs to make the trace with MASK_ALL
				if( contentmask & CONTENTS_TRIGGER )
					contentmask = MASK_ALL;

				module.trap_CM_TransformedBoxTrace( trace, start, end, mins, maxs, cmodel, contentmask, state->ms.origin, angles );
				return;
			}
		}
	}

	// the entity wasn't traceable, return a no-touch trace_t
	memset( trace, 0, sizeof( trace_t ) );
	trace->fraction = 1;
	trace->ent = -1;
}

//=================
//GS_TraceToEntities
//=================
static void GS_TraceToEntities( trace_t *tr, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask, int timeDelta )
{
	int i, j;
	entity_state_t *clipState;
	trace_t	trace;

	if( !trace_list.numEntities )
		return;

	for( j = 0; j < trace_list.numEntities; j++ )
	{
		i = trace_list.entityNums[j];

		// world doesn't count
		if( i == ENTITY_WORLD || i == passent )
			continue;

		clipState = module.GetClipStateForDeltaTime( i, timeDelta );
		if( !clipState )
			continue;

		// check contact with this entity
		GS_TraceToEntity( &trace, start, mins, maxs, end, passent, contentmask, clipState );
		if( trace.allsolid || trace.fraction < tr->fraction )
		{
			trace.ent = clipState->number;
			*tr = trace;
		}
		else if( trace.startsolid )
		{
			tr->startsolid = qtrue;
		}

		if( tr->allsolid || tr->fraction == 0 )  // don't continue if full blocked
			return;
	}
}

//=================
//GS_Trace
//=================
void GS_Trace( trace_t *tr, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int ignore, int contentmask, int timeDelta )
{
	if( !mins ) mins = vec3_origin;
	if( !maxs ) maxs = vec3_origin;

	if( ignore == ENTITY_WORLD )
	{
		memset( tr, 0, sizeof( trace_t ) );
		tr->fraction = 1;
		tr->ent = -1;
	}
	else
	{   // check against world
		module.trap_CM_TransformedBoxTrace( tr, start, end, mins, maxs, NULL, contentmask, NULL, NULL );
		tr->ent = ( tr->fraction < 1.0 ) ? ENTITY_WORLD : -1;
		if( tr->allsolid || tr->fraction == 0 )
			return;
	}

	// ask the space partition for a list of entities to test against
	GS_OctreeCreatePotentialCollideList( &trace_list, start, mins, maxs, end );

	GS_TraceToEntities( tr, start, mins, maxs, end, ignore, contentmask, timeDelta );
}
