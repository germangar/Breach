/*
Copyright (C) 2007 German Garcia
*/

#include "gs_local.h"

//==================================================
//	CLIENT MOVEMENT INPUT
//
// Find out object acceleration from the client the key impulses
//==================================================

/*
* ClampAccelToMaxSpeed
*/
static void ClampAccelToMaxSpeed( vec3_t velocity, vec3_t accel, float maxspeed )
{
	vec3_t newvelocity;
	float fullspeed;
	if( maxspeed > 0.0f )
	{
		VectorAdd( velocity, accel, newvelocity );
		fullspeed = VectorNormalize( newvelocity );
		if( fullspeed > maxspeed )
		{
			fullspeed = maxspeed;
		}

		accel[0] = ( newvelocity[0] * fullspeed ) - velocity[0];
		accel[1] = ( newvelocity[1] * fullspeed ) - velocity[1];
		accel[2] = ( newvelocity[2] * fullspeed ) - velocity[2];
	}
}

/*
* GS_Client_SetBaseAngles - base reference of mouse angles. Modify when entity is rotated from other than the user
*/
static void GS_Client_SetBaseAngles( player_state_t *playerState, vec3_t angles, short ucmdAngles[3] )
{
	//#define WARN_BASE_CHANGED
#ifdef WARN_BASE_CHANGED
	short old[3];
	VectorCopy( playerState->delta_angles, old );
#endif

	playerState->delta_angles[0] = ANGLE2SHORT( angles[0] ) - ucmdAngles[0];
	playerState->delta_angles[1] = ANGLE2SHORT( angles[1] ) - ucmdAngles[1];
	playerState->delta_angles[2] = ANGLE2SHORT( angles[2] ) - ucmdAngles[2];

#ifdef WARN_BASE_CHANGED
	if( !VectorCompare( old, playerState->delta_angles ) && gs.module == GS_MODULE_GAME )
		GS_Printf( S_COLOR_RED"Client base angles changed"S_COLOR_WHITE"\n" );
#endif
}

/*
* GS_Client_ControlForEnvironment
*/
static float GS_Client_ControlForEnvironment( moveenvironment_t *env, const gsplayerclass_t *playerClass, int contentmask )
{
	float controlFrac = playerClass->controlFracGround;

	// if it's not being clipped (freefly cam) we always apply "ground" values
	if( contentmask )
	{
		if( env->waterlevel & WATERLEVEL_FLOAT )
		{
			controlFrac = playerClass->controlFracWater;
		}
		else if( env->groundentity == ENTITY_INVALID )
		{
			controlFrac = playerClass->controlFracAir;
		}
		else
		{
			controlFrac = playerClass->controlFracGround;
			if( env->groundsurfFlags & SURF_SLICK )
				controlFrac *= 0.1f;
		}
	}

	return controlFrac;
}

static void GS_Client_UpdateZoom( entity_state_t *state, player_state_t *playerState, usercmd_t *ucmd, float fov, float zoomfov )
{
#define ZOOMTIME 150
	assert( fov > 0.0f );

	if( state->ms.type != MOVE_TYPE_STEP )
	{
		playerState->controlTimers[USERINPUT_STAT_ZOOMTIME] = 0;
	}
	else if( ( ucmd->buttons & BUTTON_ZOOM ) )
	{
		playerState->controlTimers[USERINPUT_STAT_ZOOMTIME] += ucmd->msec * 2;
		clamp( playerState->controlTimers[USERINPUT_STAT_ZOOMTIME], 0, ZOOMTIME );
	}
	else 
	{
		clamp( playerState->controlTimers[USERINPUT_STAT_ZOOMTIME], 0, ZOOMTIME );
	}

	// set fov
	if( playerState->controlTimers[USERINPUT_STAT_ZOOMTIME] <= 0 )
		playerState->fov = fov;
	else
	{
		float frac = (float)playerState->controlTimers[USERINPUT_STAT_ZOOMTIME] / (float)ZOOMTIME;
		playerState->fov = fov - ( ( fov - zoomfov ) * frac );
	}

	assert( playerState->fov > 0.0f );
#undef ZOOMTIME
}

/*
* GS_Client_UpdatePlayerBox
*/
static void GS_Client_UpdatePlayerBox( entity_state_t *state, player_state_t *playerState, usercmd_t *ucmd, const gsplayerclass_t *playerClass, moveenvironment_t *env, int contentmask )
{
#define CROUCHTIME 100
#define PRONETIME 800
	qboolean can_duck;
	qboolean crouching = qfalse;

	if( !ucmd->msec )
		return; // keep the same ones

	// mins are always the same, no matter what
	VectorCopy( playerClass->mins, state->local.mins );


	can_duck = ( qboolean )( ( env->groundentity != ENTITY_INVALID ) && !( env->waterlevel & WATERLEVEL_FLOAT ) );
	crouching = ( qboolean )( ( ucmd->upmove < 0 ) && ( playerClass->movement_features & PMOVEFEAT_CROUCH ) );

	// update duck time for proning state
	if( playerState->stats[STAT_FLAGS] & STAT_FLAGS_PRONED )
	{
		if( !( playerClass->movement_features & PMOVEFEAT_PRONE ) )
			GS_Error( "GS_Move_UpdatePlayerBox: proning without PMOVEFEAT_PRONE\n" );

		playerState->controlTimers[USERINPUT_STAT_DUCKTIME] += ucmd->msec * 2; // one to compensate the discount made to all timers
		if( playerState->controlTimers[USERINPUT_STAT_DUCKTIME] > PRONETIME )
			playerState->controlTimers[USERINPUT_STAT_DUCKTIME] = PRONETIME;
	}

	// check for crouching and update duck time for it
	if( can_duck && crouching && ( playerState->controlTimers[USERINPUT_STAT_DUCKTIME] <= CROUCHTIME )
		&& !( playerState->stats[STAT_FLAGS] & STAT_FLAGS_PRONED ) )
	{
		playerState->controlTimers[USERINPUT_STAT_DUCKTIME] += ucmd->msec * 2; // one to compensate the discount made to all timers
		if( playerState->controlTimers[USERINPUT_STAT_DUCKTIME] > CROUCHTIME )
			playerState->controlTimers[USERINPUT_STAT_DUCKTIME] = CROUCHTIME;
	}

	// uncrouch
	if( playerState->controlTimers[USERINPUT_STAT_DUCKTIME] > 0 )
	{
		// check for switching the prone mode
		if( playerClass->movement_features & PMOVEFEAT_PRONE )
		{
			// +special button will switch it on
			if( ( ucmd->buttons & BUTTON_SPECIAL ) &&
				!( playerState->stats[STAT_FLAGS] & STAT_FLAGS_PRONED ) &&
				playerState->controlTimers[USERINPUT_STAT_DUCKTIME] == CROUCHTIME )
			{
				// skip the toggle if speed wasn't yet reduced (coming from sprinting)
				if( VectorLengthFast( state->ms.velocity ) <= playerClass->crouchspeed )
					playerState->stats[STAT_FLAGS] |= STAT_FLAGS_PRONED;
			}
			// all +special, crouch and jump buttons will switch it off
			else if( ( ( ucmd->buttons & BUTTON_SPECIAL ) || ucmd->upfrac > 0 ) && 
				playerState->controlTimers[USERINPUT_STAT_DUCKTIME] == PRONETIME )
			{
				playerState->stats[STAT_FLAGS] &= ~STAT_FLAGS_PRONED;
			}
		}

		state->local.maxs[0] = playerClass->maxs[0];
		state->local.maxs[1] = playerClass->maxs[1];

		// update the height for the current state
		if( playerState->controlTimers[USERINPUT_STAT_DUCKTIME] > CROUCHTIME )
		{
			float frac = (float)(playerState->controlTimers[USERINPUT_STAT_DUCKTIME] - CROUCHTIME) / (float)(PRONETIME - CROUCHTIME);
			state->local.maxs[2] = playerClass->crouch_height + ( ( playerClass->prone_height - playerClass->crouch_height ) * frac );
			state->effects |= EF_PLAYER_PRONED;
			state->effects |= EF_PLAYER_CROUCHED;
		}
		else
		{
			float frac = (float)playerState->controlTimers[USERINPUT_STAT_DUCKTIME] / (float)CROUCHTIME;
			state->local.maxs[2] = playerClass->maxs[2] + ( ( playerClass->crouch_height - playerClass->maxs[2] ) * frac );
			state->effects |= EF_PLAYER_CROUCHED;
			state->effects &= ~EF_PLAYER_PRONED;
		}
	}
	else
	{
		state->effects &= ~EF_PLAYER_CROUCHED;

		// stand up
		if( !VectorCompare( playerClass->maxs, state->local.maxs ) )
		{
			vec3_t maxs, dest;
			trace_t trace;
			VectorCopy( state->local.maxs, maxs );
			VectorCopy( state->ms.origin, dest );
			dest[2] += ( playerClass->maxs[2] - playerClass->crouch_height );
			GS_Trace( &trace, state->ms.origin, state->local.mins, maxs, dest, state->number, contentmask, 0 );
			if( trace.ent == ENTITY_INVALID )
			{
				// be uncrouched
				VectorCopy( playerClass->maxs, state->local.maxs );
			}
			else
			{
				// be crouched
				state->local.maxs[0] = playerClass->maxs[0];
				state->local.maxs[1] = playerClass->maxs[1];
				state->local.maxs[2] = playerClass->crouch_height;
				state->effects |= EF_PLAYER_CROUCHED;
			}
		}
	}
#undef CROUCHTIME
#undef PRONETIME
}

/*
* GS_Client_FreeFly_UCmdAccel - Add acceleration in freefly styled movements
*/
static void GS_Client_FreeFly_UCmdAccel( entity_state_t *state, usercmd_t *ucmd, 
										vec3_t newaccel, float frametime, 
										moveenvironment_t *env, int contentMask )
{
	float accelspeed, maxspeed, controlFrac;
	vec3_t move_axis[3];
	gsplayerclass_t *playerClass;

	// User intended acceleration

	playerClass = GS_PlayerClassByIndex( state->playerclass );
	if( !playerClass )
		playerClass = GS_PlayerClassByIndex( 0 );

	controlFrac = GS_Client_ControlForEnvironment( env, playerClass, contentMask );
	accelspeed = playerClass->accelspeed * controlFrac;
	maxspeed = playerClass->runspeed;

	AngleVectors( state->ms.angles, move_axis[FORWARD], move_axis[RIGHT], move_axis[UP] );

	//   +v = a * t   ||   a = F / m   ||   +v = (F /m) * t
	VectorMA( newaccel, ucmd->forwardfrac * frametime * accelspeed, move_axis[FORWARD], newaccel );
	VectorMA( newaccel, ucmd->sidefrac * frametime * accelspeed, move_axis[RIGHT], newaccel );
	VectorMA( newaccel, ucmd->upfrac * frametime * accelspeed, move_axis[UP], newaccel );

	// speed limit check
	ClampAccelToMaxSpeed( state->ms.velocity, newaccel, maxspeed );
}

/*
* GS_Client_MaxWalkSpeedForState
*/
static float GS_Client_MaxWalkSpeedForState( entity_state_t *state, usercmd_t *ucmd, moveenvironment_t *env )
{
	gsplayerclass_t *playerClass;
	float maxspeed;

	playerClass = GS_PlayerClassByIndex( state->playerclass );
	if( !playerClass )
		playerClass = GS_PlayerClassByIndex( 0 );

	if( env->groundentity == ENTITY_INVALID ) // when not on ground allow to keep speed
	{
		float oldSpeed = VectorLengthFast( tv( state->ms.velocity[0], state->ms.velocity[1], 0 ) );
		maxspeed = max( oldSpeed, playerClass->runspeed );
	}
	else if( state->effects & EF_PLAYER_PRONED )
		maxspeed = playerClass->pronespeed;
	else if( state->effects & EF_PLAYER_CROUCHED )
		maxspeed = playerClass->crouchspeed;
	else if( ( ucmd->buttons & BUTTON_SPECIAL ) && ( playerClass->movement_features & PMOVEFEAT_SPRINT ) )
		maxspeed = playerClass->sprintspeed;
	else
		maxspeed = playerClass->runspeed;

	// holding heavy weapons may slow the player down

	if( maxspeed > playerClass->crouchspeed )
	{
		// todo: assign weights to all weapons
		if( state->weapon == WEAP_SNIPERRIFLE )
			maxspeed = playerClass->crouchspeed;
	}

	return maxspeed;
}

/*
* GS_Client_MoveStep_UCmdAccel - Add acceleration in gravity style movements
*/
static void GS_Client_MoveStep_UCmdAccel( entity_state_t *state, usercmd_t *ucmd, 
										 vec3_t newaccel, float frametime, 
										 moveenvironment_t *env, int contentMask )
{
	float accelspeed, maxspeed, controlFrac;
	vec3_t move_axis[3];
	vec3_t yaw_angles, pushdir;
	float forwardPush, rightPush, pushStrenght;
	gsplayerclass_t *playerClass;
	int i;

	playerClass = GS_PlayerClassByIndex( state->playerclass );
	if( !playerClass )
		playerClass = GS_PlayerClassByIndex( 0 );

	controlFrac = GS_Client_ControlForEnvironment( env, playerClass, contentMask );
	maxspeed = GS_Client_MaxWalkSpeedForState( state, ucmd, env );
	accelspeed = playerClass->accelspeed;

	// User intended acceleration

	VectorSet( yaw_angles, 0, state->ms.angles[YAW], 0 );
	AngleVectors( yaw_angles, move_axis[FORWARD], move_axis[RIGHT], move_axis[UP] );

	forwardPush = ucmd->forwardfrac * frametime * accelspeed;
	rightPush = ucmd->sidefrac * frametime * accelspeed;

	for( i = 0 ; i < 3 ; i++ )
		pushdir[i] = ( move_axis[FORWARD][i] * forwardPush ) + ( move_axis[RIGHT][i] * rightPush );

	pushStrenght = VectorNormalize( pushdir ) * controlFrac;

	if( pushStrenght )
	{
		vec3_t curDir;
		float curSpeed;
		vec3_t frictionedVelocity;

		// check max speed by adding accel to the velocity after friction, 
		// cause that's how it's going to happen later
		VectorCopy( state->ms.velocity, frictionedVelocity );
		GS_Move_ApplyFrictionToVector( frictionedVelocity, state->ms.velocity, env->frictionFrac, frametime, qfalse );

		VectorSet( curDir, frictionedVelocity[0], frictionedVelocity[1], 0 );
		curSpeed = VectorNormalize( curDir );

		// see if we are adding or removing speed
		if( DotProduct( curDir, pushdir ) < -0.0f ) // removing
		{
			VectorScale( pushdir, pushStrenght, newaccel );
		}
		else if( curSpeed < maxspeed ) // adding
		{
			// clamp to max when going to exceed it
			if( curSpeed + pushStrenght > maxspeed )
				pushStrenght = maxspeed - curSpeed;

			VectorScale( pushdir, pushStrenght, newaccel );
		}
	}
}

/*
* GS_Client_CheckJump
*/
static void GS_Client_CheckJump( entity_state_t *state, usercmd_t *ucmd, 
										 vec3_t newaccel, float frametime, 
										 moveenvironment_t *env )
{
	gsplayerclass_t *playerClass;

	if( state->effects & (EF_PLAYER_CROUCHED|EF_PLAYER_PRONED) )
		return;

	playerClass = GS_PlayerClassByIndex( state->playerclass );
	if( !playerClass )
		playerClass = GS_PlayerClassByIndex( 0 );

	if( !( playerClass->movement_features & PMOVEFEAT_JUMP ) )
		return;

	if( ( ( ucmd->upfrac * frametime ) > 0.005f ) && ( env->groundentity != ENTITY_INVALID ) )
	{
		float gspeed = DotProduct( gs.environment.gravityDir, state->ms.velocity );
		if( gspeed >= 0.0f )
		{
			GS_ClipVelocity( state->ms.velocity, gs.environment.inverseGravityDir, state->ms.velocity );
			VectorMA( vec3_origin, playerClass->jumpspeed, gs.environment.inverseGravityDir, newaccel );
		}
		else
		{
			VectorMA( newaccel, playerClass->jumpspeed, gs.environment.inverseGravityDir, newaccel );
		}

		module.predictedEvent( state->number, EV_JUMP, 0 );
	}
}

/*
* GS_Client_ApplyUserInput - Handle client input to control the character
*/
void GS_Client_ApplyUserInput( vec3_t newaccel, entity_state_t *state, 
							  player_state_t *playerState, usercmd_t *usercmd,
							  float fov, float zoomfov,
							  int contentMask, int timeDelta )
{
	int i;
	gsplayerclass_t *playerClass;
	movespecificts_t *specifics;
	moveenvironment_t env;
	usercmd_t ucmd;
	float frameTime;
	qboolean clientControl;

	assert( usercmd && playerState && state );
	assert( fov > 0.0f );

	playerClass = GS_PlayerClassByIndex( state->playerclass );
	if( !playerClass )
		playerClass = GS_PlayerClassByIndex( 0 );

	specifics = playerClass->movespec;
	if( !specifics )
		specifics = &defaultPlayerMoveSpec;

	VectorClear( newaccel );

	// adjust the ucmd in case of timer limitations
	memcpy( &ucmd, usercmd, sizeof( usercmd_t ) );

	// waste timers time
	for( i = 0; i < USERINPUT_STAT_TOTAL; i++ )
	{
		if( playerState->controlTimers[i] > 0 ) 
			playerState->controlTimers[i] -= ucmd.msec;
		else
			playerState->controlTimers[i] = 0;
	}

	clientControl = ( qboolean )!( playerState->controlTimers[USERINPUT_STAT_NOUSERCONTROL] > 0 
		|| ( ucmd.buttons & BUTTON_BUSYICON ) 
		|| ( playerState->stats[STAT_FLAGS] & STAT_FLAGS_NOINPUT ) );

	if( !clientControl )
	{
		ucmd.forwardfrac = ucmd.sidefrac = ucmd.upfrac = 0;
		ucmd.forwardmove = ucmd.sidemove = ucmd.upmove = 0;
		ucmd.buttons = ( ucmd.buttons & BUTTON_BUSYICON );
	}

	frameTime = (float)ucmd.msec * 0.001f; // convert time to seconds

	// validate its position in the world
	memset( &env, 0, sizeof( moveenvironment_t ) );

	// categorize environment for finding accel
	GS_Move_EnvironmentForBox( &env, state->ms.origin, state->ms.velocity, state->local.mins, state->local.maxs, state->number, contentMask, specifics );

	// mins and maxs are locked by now
	GS_Client_UpdatePlayerBox( state, playerState, &ucmd, playerClass, &env, contentMask );
	playerState->viewHeight = PlayerViewHeightFromBox( state->local.mins, state->local.maxs );

	GS_Client_UpdateZoom( state, playerState, &ucmd, fov, zoomfov );

	// set up movement angles from ucmd
	if( clientControl )
		GS_ClampPlayerAngles( ucmd.angles, playerState->delta_angles, state->ms.angles );

	// always update the base angles to make sure reference orientation remains correct.
	GS_Client_SetBaseAngles( playerState, state->ms.angles, ucmd.angles );

	// generate acceleration vector from user input
	if( ucmd.msec > 0 )
	{
		// do not bother finding acceleration if no button is pressed
		if( ucmd.forwardmove || ucmd.sidemove || ucmd.upmove || ( ucmd.buttons & ~BUTTON_BUSYICON ) )
		{
			switch( state->ms.type )
			{
			case MOVE_TYPE_NONE:
				break;

			case MOVE_TYPE_STEP:
				if( env.waterlevel & WATERLEVEL_FLOAT )
					GS_Client_FreeFly_UCmdAccel( state, &ucmd, newaccel, frameTime, &env, contentMask );
				else
				{
					GS_Client_MoveStep_UCmdAccel( state, &ucmd, newaccel, frameTime, &env, contentMask );
					GS_Client_CheckJump( state, &ucmd, newaccel, frameTime, &env );
				}
				break;

			case MOVE_TYPE_FREEFLY:
				GS_Client_FreeFly_UCmdAccel( state, &ucmd, newaccel, frameTime, &env, contentMask );
				break;

			case MOVE_TYPE_OBJECT:
				GS_Client_MoveStep_UCmdAccel( state, &ucmd, newaccel, frameTime, &env, contentMask );
				break;

			case MOVE_TYPE_LINEAR:
			case MOVE_TYPE_PUSHER:
			case MOVE_TYPE_LINEAR_PROJECTILE:
				break;

			default:
				GS_Error( "GS_Client_ApplyUserInput: Invalid movetype\n" );
				break;
			}	
		}
	}

	// check for activate button trigger
	if( ucmd.buttons & BUTTON_ACTIVATE )
	{
		// by now lets do it only from the game module (no need to run it for each ucmd prediction)
		if( module.type == GS_MODULE_GAME )
		{
			int targetNum = GS_FindActivateTargetInFront( state, playerState, timeDelta );
			if( targetNum != ENTITY_INVALID )
				module.predictedEvent( state->number, EV_ACTIVATE, targetNum );
		}
	}

	// apply user input to player weapon
	GS_ThinkPlayerWeapon( state, playerState, &ucmd );
}
