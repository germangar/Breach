/*
   Copyright (C) 2007 German Garcia
 */

//==================================================
// COLLISION MODELS
//==================================================

extern void GS_DecodeEntityBBox( int solid, vec3_t mins, vec3_t maxs );
extern int GS_EncodeEntityBBox( vec3_t mins, vec3_t maxs );
extern qboolean GS_IsBrushModel( int modelindex );
extern void GS_AbsoluteBoundsForEntity( entity_state_t *state );
extern void GS_CenterOfEntity( vec3_t currentOrigin, entity_state_t *state, vec3_t origin );
extern struct cmodel_s *GS_CModelForEntity( entity_state_t *state );
extern void GS_RelativeBoxForBrushModel( struct cmodel_s *cmodel, vec3_t box_origin, vec3_t box_mins, vec3_t box_maxs );

//==================================================
// CROSSHAIR ACTIVATION OF ENTITIES
//==================================================
extern int GS_FindActivateTargetInFront( entity_state_t *state, player_state_t *playerState, int timeDelta );

//==================================================
// INSTANT HIT BULLET TRACING
//==================================================
extern void GS_SpreadDir( vec3_t inDir, vec3_t outDir, int hspread, int vspread, int *seed );
extern void GS_SeedFireBullet( trace_t *trace, vec3_t start, vec3_t dir, int range,
                              int hspread, int vspread, int *seed, int ignore, int timeDelta,
                              void ( *impactEvent )( vec3_t frstart, trace_t *trace, qboolean water, qboolean transition ) );

//==================================================
// MISC
//==================================================

#define	MAXTOUCH    32

typedef struct
{
	int numtouch;
	int touchents[MAXTOUCH];
	cplane_t touchplanes[MAXTOUCH];
	int touchsurfs[MAXTOUCH];
} touchlist_t;

extern float GS_FrameForTime( int *frame, unsigned int curTime, unsigned int startTimeStamp, float frametime, int firstframe, int lastframe, int loopingframes, qboolean forceLoop );
extern int GS_WaterLevelForBBox( vec3_t origin, vec3_t mins, vec3_t maxs, int *watertype );
extern void GS_TouchPushTrigger( int movetype, vec3_t velocity, entity_state_t *pusher );
extern void GS_AddTouchEnt( touchlist_t *touchList, int entNum, cplane_t *plane, int surfaceflags );

//==================================================
// VECTOR SNAPPING
//==================================================

extern qboolean GS_SnapPosition( vec3_t origin, vec3_t mins, vec3_t maxs, int passent, int contentmask );
extern qboolean GS_SnapInitialPosition( vec3_t origin, vec3_t mins, vec3_t maxs, int passent, int contentmask );
extern void GS_SnapVector( vec3_t velocity );
extern void GS_ClampPlayerAngles( short cmd_angles[3], short delta_angles[3], float angles[3] );
extern void GS_BounceVelocity( vec3_t velocity, vec3_t normal, vec3_t newvelocity, float overbounce );
extern void GS_ClipVelocity( vec3_t velocity, vec3_t normal, vec3_t newvelocity );
