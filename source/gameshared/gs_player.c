/*
Copyright (C) 2007 German Garcia
*/

#include "gs_local.h"

//==================================================
//	PLAYER PHYSICS MODEL
//==================================================

movespecificts_t defaultPlayerMoveSpec = {
	1550,    // frictionGround
	25,     // frictionAir
	600,    // frictionWater
	0,      // bounceFrac
};

//==================================================
//	PLAYER CLASSES
//==================================================

static gsplayerclass_t playerClassNoClass =
{
	"noclass",
	0,
	100, // initial health
	100, // max health

	"",
	0,  // playerObject index

	&defaultPlayerMoveSpec,
	{ -16, -16, -24 }, // box mins
	{ 16, 16, 40 }, // box maxs
	0,		// movement features
	16,		// crouch height
	0,		// prone height
	300,	// run-speed
	450,    // sprint-speed
	175,    // crouch-speed
	75,    // prone-speed
	350,    // jumpspeed
	3000,   // accelspeed
	1,      // controlFracGround
	0.2f,  // controlFracAir
	0.6f,   // controlFracWater

	// locally derived information
	NULL,
};

static gsplayerclass_t *playerClassHeadNode = NULL;

/*
* GS_PlayerClassByName
*/
gsplayerclass_t *GS_PlayerClassByName( const char *name )
{
	gsplayerclass_t *playerClass;

	if( name && name[0] )
	{
		for( playerClass = playerClassHeadNode; playerClass != NULL; playerClass = playerClass->next )
		{
			if( !Q_stricmp( playerClass->classname, name ) )
				return playerClass;
		}

		if( !Q_stricmp( name, playerClassNoClass.classname ) )
			return &playerClassNoClass;
	}

	return NULL;
}

/*
* GS_PlayerClassByIndex
*/
gsplayerclass_t *GS_PlayerClassByIndex( int index )
{
	gsplayerclass_t *playerClass;

	if( !index )
		return &playerClassNoClass;

	if( index > 0 && index <= gs.numPlayerClasses )
	{
		for( playerClass = playerClassHeadNode; playerClass != NULL; playerClass = playerClass->next )
		{
			if( playerClass->index == index )
				return playerClass;
		}
	}

	return NULL;
}

/*
* GS_PlayerClass_Register
*/
gsplayerclass_t *GS_PlayerClass_Register( int index, const char *classString, const char *dataString )
{
	gsplayerclass_t *playerClass;
	char name[MAX_QPATH], object[MAX_QPATH];
	const char *s;

	int mins0, mins1, mins2, maxs0, maxs1, maxs2, features, crouchHeight, proneHeight;
	int runSpeed, sprintSpeed, crouchSpeed, proneSpeed, jumpSpeed, accelSpeed;
	float controlFracGround, controlFracAir, controlFracWater;
	int i, objectIndex;

	if( !classString || !classString[0] || !dataString || !dataString[0] )
		return NULL;

	// class string

	s = Info_ValueForKey( classString, "n" );
	if( !s || !s[0] )
		GS_Error( "GS_PlayerClass_Register: Tried to precache playerclass without a name\n" );
	Q_strncpyz( name, s, sizeof( name ) );

	s = Info_ValueForKey( classString, "o" );
	if( !s || !s[0] )
		GS_Error( "GS_PlayerClass_Register: Tried to precache playerclass without an object\n" );
	Q_strncpyz( object, s, sizeof( object ) );

	s = Info_ValueForKey( classString, "p" );
	if( !s || !s[0] )
		GS_Error( "GS_PlayerClass_Register: Tried to precache playerclass without an object index\n" );
	objectIndex = atoi( s );

	// data string

	i = sscanf( dataString, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f",
		&mins0, &mins1, &mins2,
		&maxs0, &maxs1, &maxs2,
		&features,
		&crouchHeight,
		&proneHeight,
		&runSpeed, &sprintSpeed, &crouchSpeed,
		&proneSpeed, &jumpSpeed, &accelSpeed,
		&controlFracGround,
		&controlFracAir,
		&controlFracWater );

	// we only accept the data if the string is perfect
	if( i != 18 )
		GS_Error( "GS_PlayerClass_Register: Invalid dataString string: %s (%i tokens)\n", dataString, i );

	// see if it was already registered
	playerClass = GS_PlayerClassByIndex( index );
	if( playerClass != NULL )
	{
		if( playerClass->classname )
			module_Free( playerClass->classname );
		if( playerClass->playerObject )
			module_Free( playerClass->playerObject );
	}
	else
	{
		// create the new playerClass container
		playerClass = ( gsplayerclass_t * )module_Malloc( sizeof( gsplayerclass_t ) );
		playerClass->next = playerClassHeadNode;
		playerClassHeadNode = playerClass;

		gs.numPlayerClasses++;
		if( index > gs.numPlayerClasses )
			GS_Error( "GS_PlayerClass_Register: index > gs.numPlayerClasses\n" );
	}

	playerClass->movespec = &defaultPlayerMoveSpec; // this is the same for all by now

	playerClass->index = index;
	playerClass->objectIndex = objectIndex;
	playerClass->classname = GS_CopyString( name );
	playerClass->playerObject = GS_CopyString( object );

	playerClass->mins[0] = mins0;
	playerClass->mins[1] = mins1;
	playerClass->mins[2] = mins2;
	playerClass->maxs[0] = maxs0;
	playerClass->maxs[1] = maxs1;
	playerClass->maxs[2] = maxs2;
	playerClass->movement_features = features;
	playerClass->crouch_height = crouchHeight;
	playerClass->prone_height = proneHeight;
	playerClass->runspeed = runSpeed;
	playerClass->sprintspeed = sprintSpeed;
	playerClass->crouchspeed = crouchSpeed;
	playerClass->pronespeed = proneSpeed;
	playerClass->jumpspeed = jumpSpeed;
	playerClass->accelspeed = accelSpeed;
	playerClass->controlFracGround = controlFracGround;
	playerClass->controlFracAir = controlFracAir;
	playerClass->controlFracWater = controlFracWater;

	return playerClass;
}

/*
* GS_PlayerClass_FreeAll
*/
void GS_PlayerClass_FreeAll( void )
{
	gsplayerclass_t *playerClass;

	playerClass = playerClassHeadNode;

	while( playerClass )
	{
		playerClassHeadNode = playerClass->next;
		if( playerClass->classname )
			module_Free( playerClass->classname );
		if( playerClass->playerObject )
			module_Free( playerClass->playerObject );
		module_Free( playerClass );
		playerClass = playerClassHeadNode;
	}

	gs.numPlayerClasses = 0;
}

//==================================================
//	PLAYER ANIMATIONS
//==================================================

#define MOVEDIREPSILON	0.3f
#define WALKEPSILON	5.0f
#define RUNEPSILON	220.0f

// movement flags for animation control
#define ANIMMOVE_FRONT		0x00000001  //	Player is pressing fordward
#define ANIMMOVE_BACK		0x00000002  //	Player is pressing backpedal
#define ANIMMOVE_LEFT		0x00000004  //	Player is pressing sideleft
#define ANIMMOVE_RIGHT		0x00000008  //	Player is pressing sideright
#define ANIMMOVE_WALK		0x00000010  //	Player is pressing the walk key
#define ANIMMOVE_RUN		0x00000020  //	Player is running
#define ANIMMOVE_DUCK		0x00000040  //	Player is crouching
#define ANIMMOVE_SWIM		0x00000080  //	Player is swimming
#define ANIMMOVE_AIR		0x00000100  //	Player is at air, but not jumping

typedef struct
{
	int moveflags;
	int animState[PMODEL_PARTS];
} pm_anim_t;


/*
* GS_SetBaseAnimUpper
*/
static void GS_SetBaseAnimUpper( pm_anim_t *pmanim )
{
	// SWIMMING
	if( pmanim->moveflags & ANIMMOVE_SWIM )
	{
		pmanim->animState[UPPER] = TORSO_SWIM;
	}
	// FALLING
	else if( pmanim->moveflags & ANIMMOVE_AIR )
	{
		pmanim->animState[UPPER] = TORSO_IDLE;
	}
	// CROUCH
	else if( pmanim->moveflags & ANIMMOVE_DUCK )
	{
		if( pmanim->moveflags & ( ANIMMOVE_WALK|ANIMMOVE_RUN ) )
		{
			pmanim->animState[UPPER] = TORSO_RUN;
		}
		else
		{
			pmanim->animState[UPPER] = TORSO_IDLE;
		}
	}
	// RUN
	else if( pmanim->moveflags & ANIMMOVE_RUN )
	{
		pmanim->animState[UPPER] = TORSO_RUN;
	}
	// WALK
	else if( pmanim->moveflags & ANIMMOVE_WALK )
	{
		pmanim->animState[UPPER] = TORSO_IDLE;
	}
	// STAND
	else
	{
		pmanim->animState[UPPER] = TORSO_IDLE;
	}
}

/*
* GS_SetBaseAnimLower
*/
static void GS_SetBaseAnimLower( pm_anim_t *pmanim )
{
	// SWIMMING
	if( pmanim->moveflags & ANIMMOVE_SWIM )
	{
		if( pmanim->moveflags & ANIMMOVE_FRONT )
		{
			pmanim->animState[LOWER] = LEGS_SWIMFWD;
		}
		else
			pmanim->animState[LOWER] = LEGS_SWIM;
	}
	// FALLING
	else if( pmanim->moveflags & ANIMMOVE_AIR )
	{
		pmanim->animState[LOWER] = LEGS_FALLING;
	}
	// CROUCH
	else if( pmanim->moveflags & ANIMMOVE_DUCK )
	{
		if( pmanim->moveflags & ( ANIMMOVE_WALK|ANIMMOVE_RUN ) )
		{
			pmanim->animState[LOWER] = LEGS_CRWALK;
		}
		else
		{
			pmanim->animState[LOWER] = LEGS_IDLECR;
		}
	}
	// RUN
	else if( pmanim->moveflags & ANIMMOVE_RUN )
	{
		// front/backward has priority over side movements
		if( pmanim->moveflags & ANIMMOVE_FRONT )
		{
			pmanim->animState[LOWER] = LEGS_RUNFWD;

		}
		else if( pmanim->moveflags & ANIMMOVE_BACK )
		{
			pmanim->animState[LOWER] = LEGS_RUNBACK;

		}
		else if( pmanim->moveflags & ANIMMOVE_RIGHT )
		{
			pmanim->animState[LOWER] = LEGS_RUNRIGHT;

		}
		else if( pmanim->moveflags & ANIMMOVE_LEFT )
		{
			pmanim->animState[LOWER] = LEGS_RUNLEFT;

		}
		else  // is moving by inertia
			pmanim->animState[LOWER] = LEGS_WALKFWD;
	}
	// WALK
	else if( pmanim->moveflags & ANIMMOVE_WALK )
	{
		// front/backward has priority over side movements
		if( pmanim->moveflags & ANIMMOVE_FRONT )
		{
			pmanim->animState[LOWER] = LEGS_WALKFWD;

		}
		else if( pmanim->moveflags & ANIMMOVE_BACK )
		{
			pmanim->animState[LOWER] = LEGS_WALKBACK;

		}
		else if( pmanim->moveflags & ANIMMOVE_RIGHT )
		{
			pmanim->animState[LOWER] = LEGS_WALKRIGHT;

		}
		else if( pmanim->moveflags & ANIMMOVE_LEFT )
		{
			pmanim->animState[LOWER] = LEGS_WALKLEFT;

		}
		else  // is moving by inertia
			pmanim->animState[LOWER] = LEGS_WALKFWD;
	}
	else
	{   // STAND
		pmanim->animState[LOWER] = LEGS_IDLE;
	}
}

/*
* GS_SetBaseAnims
*/
static void GS_SetBaseAnims( pm_anim_t *pmanim )
{
	int part;

	for( part = 0; part < PMODEL_PARTS; part++ )
	{
		switch( part )
		{
		case LOWER:
			GS_SetBaseAnimLower( pmanim );
			break;

		case UPPER:
			GS_SetBaseAnimUpper( pmanim );
			break;

		case HEAD:
		default:
			pmanim->animState[part] = 0;
			break;
		}
	}
}

/*
* GS_UpdateBaseAnims
*/
int GS_UpdateBaseAnims( entity_state_t *state, vec3_t velocity, int *footstep_type )
{
	pm_anim_t pmanim;
	vec3_t movedir;
	vec3_t hvel;
	vec3_t viewaxis[3];
	float xyspeedcheck;
	int waterlevel;
	vec3_t point;
	trace_t	trace;
	int stepSurfaceFlags = 0;

	if( !state )
		GS_Error( "GS_UpdateBaseAnims: NULL state\n" );

	memset( &pmanim, 0, sizeof( pm_anim_t ) );

	// determine if player is at ground, for walking or falling
	// this is not like having groundEntity, we are more generous with
	// the tracing size here to include small steps
	VectorMA( state->ms.origin, 1.75 * STEPSIZE, gs.environment.gravityDir, point );
	GS_Trace( &trace, state->ms.origin, state->local.mins, state->local.maxs, point, state->number, MASK_PLAYERSOLID, 0 );
	if( trace.ent == ENTITY_INVALID || ( trace.fraction < 1.0f && !IsGroundPlane( trace.plane.normal, gs.environment.gravityDir ) ) )
	{
		pmanim.moveflags |= ANIMMOVE_AIR;
	}
	else
		stepSurfaceFlags = trace.surfFlags;

	if( state->effects & EF_PLAYER_CROUCHED )
	{
		pmanim.moveflags |= ANIMMOVE_DUCK;
	}

	// find out the water level
	// FIXME!!! : if the entity state environment is up to date, we don't need to double check waterlevel
	waterlevel = GS_WaterLevelForBBox( state->ms.origin, state->local.mins, state->local.maxs, NULL );
	if( waterlevel & WATERLEVEL_FLOAT || ( waterlevel && ( pmanim.moveflags & ANIMMOVE_AIR ) ) )
	{
		pmanim.moveflags |= ANIMMOVE_SWIM;
	}

	//find out what are the base movements the model is doing

	hvel[0] = velocity[0];
	hvel[1] = velocity[1];
	hvel[2] = 0;
	xyspeedcheck = VectorNormalize2( hvel, movedir );
	if( xyspeedcheck > WALKEPSILON )
	{
		VectorNormalizeFast( movedir );
		AngleVectors( tv( 0, state->ms.angles[YAW], 0 ), viewaxis[FORWARD], viewaxis[RIGHT], viewaxis[UP] );

		// if it's moving to where is looking, it's moving forward
		if( DotProduct( movedir, viewaxis[RIGHT] ) > MOVEDIREPSILON )
		{
			pmanim.moveflags |= ANIMMOVE_RIGHT;
		}
		else if( -DotProduct( movedir, viewaxis[RIGHT] ) > MOVEDIREPSILON )
		{
			pmanim.moveflags |= ANIMMOVE_LEFT;
		}
		if( DotProduct( movedir, viewaxis[FORWARD] ) > MOVEDIREPSILON )
		{
			pmanim.moveflags |= ANIMMOVE_FRONT;
		}
		else if( -DotProduct( movedir, viewaxis[FORWARD] ) > MOVEDIREPSILON )
		{
			pmanim.moveflags |= ANIMMOVE_BACK;
		}

		if( xyspeedcheck > RUNEPSILON )
			pmanim.moveflags |= ANIMMOVE_RUN;
		else if( xyspeedcheck > WALKEPSILON )
			pmanim.moveflags |= ANIMMOVE_WALK;
	}

	// set up the footstep type
	if( footstep_type )
	{
		*footstep_type = 0;
		if( !( pmanim.moveflags & ANIMMOVE_SWIM ) && !( pmanim.moveflags & ANIMMOVE_AIR ) )
		{
			if( !( stepSurfaceFlags & SURF_NOSTEPS ) )
			{
				if( pmanim.moveflags & ANIMMOVE_RUN )
				{
					*footstep_type = 1;
				}

				if( *footstep_type && waterlevel )
				{
					*footstep_type = 2;
				}
			}
		}
	}

	GS_SetBaseAnims( &pmanim );
	return ( ( pmanim.animState[LOWER] &0x3F ) | ( pmanim.animState[UPPER] &0x3F )<<6 | ( pmanim.animState[HEAD] &0xF )<<12 );
}
#undef MOVEDIREPSILON
#undef WALKEPSILON
#undef RUNEPSILON

/*
* GS_PModel_AnimToFrame
*
* BASE_CHANEL plays continuous animations forced to loop.
* if the same animation is received twice it will *not* restart
* but continue looping.
*
* EVENT_CHANNEL overrides base channel and plays until
* the animation is finished. Then it returns to base channel.
* If an animation is received twice, it will be restarted.
* If an event channel animation has a loop setting, it will
* continue playing it until a new event chanel animation
* is fired.
*/
void GS_Player_AnimToFrame( unsigned int curTime, gs_pmodel_animationset_t *animSet, gs_player_animationstate_t *anim )
{
	int i, channel = BASE_CHANNEL;
	int curframe;

	for( i = LOWER; i < PMODEL_PARTS; i++ )
	{
		for( channel = BASE_CHANNEL; channel < PLAYERANIM_CHANNELS; channel++ )
		{
			gs_animstate_t *thisAnim = &anim->curAnims[i][channel];

			// see if there are new animations to be played
			if( anim->buffer[channel].newanim[i] != ANIM_NONE )
			{
				if( channel == EVENT_CHANNEL || 
					( channel == BASE_CHANNEL && anim->buffer[channel].newanim[i] != thisAnim->anim ) )
				{
					thisAnim->anim = anim->buffer[channel].newanim[i];
					thisAnim->startTimestamp = curTime;
				}

				anim->buffer[channel].newanim[i] = ANIM_NONE;
			}

			if( thisAnim->anim )
			{
				qboolean forceLoop = (qboolean)( channel == BASE_CHANNEL );

				thisAnim->lerpFrac = GS_FrameForTime( &thisAnim->frame, curTime, thisAnim->startTimestamp,
					animSet->frametime[thisAnim->anim], animSet->firstframe[thisAnim->anim], animSet->lastframe[thisAnim->anim],
					animSet->loopingframes[thisAnim->anim], forceLoop );

				// the animation was completed
				if( thisAnim->frame < 0 )
				{
					assert( channel != BASE_CHANNEL );
					thisAnim->anim = ANIM_NONE;
					curframe = 0;
				}
			}
		}
	}

	// we set all animations up, but now select which ones are going to be shown
	for( i = LOWER; i < PMODEL_PARTS; i++ )
	{
		int lastframe = anim->frame[i];
		channel = ( anim->curAnims[i][EVENT_CHANNEL].anim != ANIM_NONE ) ? EVENT_CHANNEL : BASE_CHANNEL;
		anim->frame[i] = anim->curAnims[i][channel].frame;
		anim->lerpFrac[i] = anim->curAnims[i][channel].lerpFrac;
		if( !lastframe )
			anim->oldframe[i] = anim->frame[i];
		else if( anim->frame[i] != lastframe )
			anim->oldframe[i] = lastframe;
	}
}

/*
* GS_PModel_AddAnimation
*/
void GS_PlayerModel_AddAnimation( gs_player_animationstate_t *animState, int loweranim, int upperanim, int headanim, int channel )
{
	int i;
	int newanim[PMODEL_PARTS];
	gs_animationbuffer_t *buffer;

	assert( animState != NULL );

	newanim[LOWER] = loweranim;
	newanim[UPPER] = upperanim;
	newanim[HEAD] = headanim;

	buffer = &animState->buffer[channel];

	for( i = LOWER; i < PMODEL_PARTS; i++ )
	{
		//ignore new events if in death
		if( channel && buffer->newanim[i] && ( buffer->newanim[i] <= BOTH_DEAD1 ) )
			continue;

		if( newanim[i] && ( newanim[i] < PMODEL_TOTAL_ANIMATIONS ) )
			buffer->newanim[i] = newanim[i];
	}
}
