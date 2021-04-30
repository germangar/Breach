/*
   Copyright (C) 2007 German Garcia
 */

// private to gs_* files

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_comref.h"
#include "../gameshared/q_collision.h"

#include "gs_public.h"

extern gs_moduleapi_t module;

#define module_Malloc(size) module.Malloc(size,__FILE__,__LINE__)
#define module_Free(data) module.Free(data,__FILE__,__LINE__)

//==================================================

//==================================================

// gs_gametypes.c
extern void GS_Teams_Init( void );

//gs_utils.c
extern void GS_SnapVector( vec3_t velocity );

//gs_moveslide.c
extern void GS_BoxStepSlideMove( move_t *move );
extern void GS_BoxSlideMove( move_t *move );
extern int GS_BoxLinearMove( move_t *move );

//gs_move.c
extern void GS_Move_ApplyFrictionToVector( vec3_t vector, const vec3_t velocity, const float friction, const float frametime, const qboolean freefly );

// gs_weapons.c
extern void GS_ThinkPlayerWeapon( entity_state_t *state, player_state_t *playerState, usercmd_t *usercmd );
