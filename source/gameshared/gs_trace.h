/*
   Copyright (C) 2007 German Garcia
 */

typedef struct
{
	int numEntities;
	int contentmask;
	int entityNums[MAX_EDICTS];
} gs_traceentitieslist;

//==================================================
//	SPACE PARTITION
//==================================================

extern void GS_CreateSpacePartition( void );
extern void GS_FreeSpacePartition( void );
extern void GS_DrawSpacePartition( void );

extern qboolean GS_SpaceLinkEntity( entity_state_t *state );
extern qboolean GS_SpaceUnLinkEntity( entity_state_t *state );

extern void GS_AddTraceIgnore( int entNum );
extern void GS_ClearTraceIgnores( void );
extern void GS_OctreeCreatePotentialCollideList( gs_traceentitieslist *traceList, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end );

//==================================================
//	TRACING
//==================================================

extern int GS_ContentMaskForSolid( int solid );
extern void GS_TraceToEntity( trace_t *tr, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask, entity_state_t *state );
extern void GS_Trace( trace_t *tr, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int ignore, int contentmask, int timeDelta );
extern int GS_PointContents( vec3_t point, int timeDelta );
