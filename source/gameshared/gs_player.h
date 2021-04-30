/*
Copyright (C) 2007 German Garcia
*/

#ifndef __GS_PLAYER_H__
#define __GS_PLAYER_H__

typedef struct gsplayerclass_s
{
	char *classname;
	int index;
	int health;
	int maxHealth;

	char *playerObject;
	int objectIndex;

	movespecificts_t *movespec;
	vec3_t mins;
	vec3_t maxs;
	int movement_features;
	float crouch_height;
	float prone_height;
	float runspeed;
	float sprintspeed;
	float crouchspeed;
	float pronespeed;
	float jumpspeed;
	float accelspeed;
	float controlFracGround;
	float controlFracAir;
	float controlFracWater;

	// locally derived information
	struct gsplayerclass_s *next;
}
gsplayerclass_t;

gsplayerclass_t *GS_PlayerClassByIndex( int index );
gsplayerclass_t *GS_PlayerClassByName( const char *name );
gsplayerclass_t *GS_PlayerClass_Register( int index, const char *classString, const char *dataString );
void GS_PlayerClass_FreeAll( void );

//==================================================
// PLAYER OBJECTS
//==================================================

// gender stuff
enum
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTRAL
};

// The parts must be listed in draw order
enum
{
	LOWER = 0,
	UPPER,
	HEAD,

	PMODEL_PARTS
};

// ANIMATIONS

enum
{
	ANIM_NONE = 0
	, BOTH_DEATH1       //Death animation
	, BOTH_DEAD1        //corpse on the ground

	, LEGS_IDLE         //Stand idle

	, LEGS_WALKFWD      //WalkFordward (few frames of this will show when moving by inertia)
	, LEGS_WALKBACK     //WalkBackward
	, LEGS_WALKLEFT     //WalkLeft
	, LEGS_WALKRIGHT    //WalkRight

	, LEGS_RUNFWD       //RunFordward
	, LEGS_RUNBACK      //RunBackward
	, LEGS_RUNLEFT      //RunLeft
	, LEGS_RUNRIGHT     //RunRight

	, LEGS_IDLECR       //Crouched idle
	, LEGS_CRWALK       //Crouched Walk

	, LEGS_SWIMFWD      //Swim Fordward
	, LEGS_SWIM         //Stand & Backwards Swim

	, LEGS_FALLING      //Left leg land
	, LEGS_JUMP         //Stand & Backwards jump.

	, TORSO_IDLE        //Stand
	, TORSO_RUN         //Run (also used for jump)
	, TORSO_SWIM        //Swim

	, TORSO_ATTACK1     //Attack
	, TORSO_WEAP_DOWN   //put out current weapon
	, TORSO_WEAP_UP     //put in new weapon

	, TORSO_PAIN1       //Pain1
	, TORSO_PAIN2       //Pain2
	, TORSO_PAIN3       //Pain3

	, TORSO_TAUNT       //gesture

	, PMODEL_TOTAL_ANIMATIONS
};

typedef struct
{
	int firstframe[PMODEL_TOTAL_ANIMATIONS];                //animation script
	int lastframe[PMODEL_TOTAL_ANIMATIONS];
	int loopingframes[PMODEL_TOTAL_ANIMATIONS];
	float frametime[PMODEL_TOTAL_ANIMATIONS];
} gs_pmodel_animationset_t;

enum
{
	BASE_CHANNEL,
	EVENT_CHANNEL,

	PLAYERANIM_CHANNELS
};

typedef struct
{   // don't really need a struct for this, but I like it this way
	int newanim[PMODEL_PARTS];
} gs_animationbuffer_t;

typedef struct  
{
	int anim;
	int frame;
	unsigned int startTimestamp;
	float lerpFrac;
}gs_animstate_t;

typedef struct
{
	// animations in the mixer
	gs_animstate_t curAnims[PMODEL_PARTS][PLAYERANIM_CHANNELS];
	gs_animationbuffer_t buffer[PLAYERANIM_CHANNELS];
	int oldframeanims;

	// results
	int frame[PMODEL_PARTS];
	int oldframe[PMODEL_PARTS];
	float lerpFrac[PMODEL_PARTS];
} gs_player_animationstate_t;

extern int GS_UpdateBaseAnims( entity_state_t *state, vec3_t velocity, int *footstep_type );
extern void GS_Player_AnimToFrame( unsigned int curTime, gs_pmodel_animationset_t *animSet, gs_player_animationstate_t *anim );
extern void GS_PlayerModel_AddAnimation( gs_player_animationstate_t *animState, int loweranim, int upperanim, int headanim, int channel );

#endif // __GS_PLAYER_H__
