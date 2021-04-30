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

#include "cg_local.h"

/*
* CG_CheckPredictionError
*/
void CG_CheckPredictionError( void )
{
	int frame;
	int delta[3];

	if( !cg.view.playerPrediction )
		return;

	trap_NET_GetCurrentState( &frame, NULL, NULL );

	// calculate the last usercmd_t we sent that the server has processed
	frame = frame & CMD_MASK;

	// compare what the server returned with what we had predicted it to be
	VectorSubtract( cg_entities[cg.predictedPlayerState.POVnum].current.ms.origin, cg.predictedOrigins[frame], delta );

	// save the prediction error for interpolation
	if( abs( delta[0] ) > 128 || abs( delta[1] ) > 128 || abs( delta[2] ) > 128 )
	{
		if( cg_predict_debug->integer )
			GS_Printf( "prediction miss on %i: %i\n", cg.frame.snapNum, abs( delta[0] ) + abs( delta[1] ) + abs( delta[2] ) );
		VectorClear( cg.predictionError );          // a teleport or something
	}
	else
	{
		if( cg_predict_debug->integer && ( delta[0] || delta[1] || delta[2] ) )
			GS_Printf( "prediction miss on %i: %i\n", cg.frame.snapNum, abs( delta[0] ) + abs( delta[1] ) + abs( delta[2] ) );

		VectorCopy( cg_entities[cg.predictedPlayerState.POVnum].current.ms.origin, cg.predictedOrigins[frame] );
		VectorCopy( delta, cg.predictionError ); // save for error interpolation
	}
}

//========================================================================

/*
Copyright (C) 2007 German Garcia
*/

//========================================================================

static qboolean	cg_triggersListTriggered[MAX_SNAPSHOT_ENTITIES];
static unsigned int predictingTimeStamp;
static float predictedSteps[CMD_BACKUP];
#define PREDICTED_STEP_TIME 150

/*
* CG_PredictedEvent - shared code can fire events during prediction
*/
void CG_PredictedEvent( int entNum, int ev, int parm )
{
	if( ev >= PREDICTABLE_EVENTS_MAX )
		return;

	// ignore this action if it has already been predicted (the unclosed ucmd has timestamp zero)
	if( predictingTimeStamp && ( predictingTimeStamp > cg.predictedEventTimes[ev] ) )
	{
		// inhibit the fire event when there is a weapon change predicted
		if( ev == EV_FIREWEAPON )
		{
			if( cg.predictedNewWeapon && ( cg.predictedNewWeapon != cg.predictedPlayerState.stats[STAT_PENDING_WEAPON] ) )
				return;
		}

		cg.predictedEventTimes[ev] = predictingTimeStamp;
		CG_EntityEvent( &cg_entities[entNum].current, ev, parm, qtrue );
	}
}

/*
* CG_Predict_ChangeWeapon
*/
void CG_Predict_ChangeWeapon( int new_weapon )
{
	if( cg.view.playerPrediction )
	{
		cg.predictedNewWeapon = new_weapon;
	}
}

/*
* CG_Predict_TouchTriggers
*/
static void CG_Predict_TouchTriggers( move_t *move, int solid )
{
	int i;
	entity_state_t *state;
	trace_t	trace;
	static gs_traceentitieslist clipList;

	if( !predictingTimeStamp ) // do not check in unfinished ucmds
		return;

	if( solid == SOLID_NOT || solid == SOLID_CORPSE )
		return;

	GS_OctreeCreatePotentialCollideList( &clipList, move->ms->origin, move->local.mins, move->local.maxs, move->ms->origin );
	if( !clipList.numEntities )
		return;

	for( i = 0; i < clipList.numEntities; i++ )
	{
		if( clipList.entityNums[i] == move->entNum )
			continue;

		state = CG_GetClipStateForDeltaTime( clipList.entityNums[i], 0 );
		if( !state || state->solid != SOLID_TRIGGER )
			continue;

		if( !cg_triggersListTriggered[clipList.entityNums[i]] )
		{
			switch( state->type )
			{
			default:
				break;
			case ET_TOTAL_TYPES /*ET_PUSH_TRIGGER*/ : // FIXME
				GS_TraceToEntity( &trace, move->ms->origin, move->local.mins, move->local.maxs, move->ms->origin, move->entNum, CONTENTS_TRIGGER, state );
				if( trace.startsolid || trace.allsolid )
				{
					GS_TouchPushTrigger( move->ms->type, move->ms->velocity, state );
					cg_triggersListTriggered[clipList.entityNums[i]] = qtrue;
				}
				break;
			}
		}
	}
}

/*
* CG_PredictSmoothOriginForSteps
*/
void CG_PredictSmoothOriginForSteps( vec3_t origin )
{
	int timeDelta;

	// smooth out stair climbing
	timeDelta = cg.realTime - cg.predictedStepTime;
	if( timeDelta < PREDICTED_STEP_TIME )
		origin[2] -= cg.predictedStep * ( PREDICTED_STEP_TIME - timeDelta ) / PREDICTED_STEP_TIME;
}

/*
* CG_PredictAddStep
*/
static void CG_PredictAddStep( int virtualtime, int predictiontime, float stepSize )
{
	float oldStep;
	int delta;

	// check for stepping up before a previous step is completed
	delta = cg.realTime - cg.predictedStepTime;
	if( delta < PREDICTED_STEP_TIME )
		oldStep = cg.predictedStep * ( (float)( PREDICTED_STEP_TIME - delta ) / (float)PREDICTED_STEP_TIME );
	else
		oldStep = 0;

	cg.predictedStep = oldStep + stepSize;
	cg.predictedStepTime = cg.realTime - ( predictiontime - virtualtime );
}

/*
* CG_PredictSmoothSteps
*/
static void CG_PredictSmoothSteps( void )
{
	int ucmdHead;
	int frame;
	usercmd_t cmd;
	int i;
	int virtualtime = 0, predictiontime = 0;

	cg.predictedStepTime = 0;
	cg.predictedStep = 0;

	ucmdHead = trap_NET_GetCurrentUserCmdNum();

	i = ucmdHead;
	while( predictiontime < PREDICTED_STEP_TIME )
	{
		if( ucmdHead - i >= CMD_BACKUP )
			break;

		frame = i & CMD_MASK;
		trap_NET_GetUserCmd( frame, &cmd );
		predictiontime += cmd.msec;
		i--;
	}

	// run frames
	while( ++i <= ucmdHead )
	{
		frame = i & CMD_MASK;
		trap_NET_GetUserCmd( frame, &cmd );
		virtualtime += cmd.msec;

		if( predictedSteps[frame] )
			CG_PredictAddStep( virtualtime, predictiontime, predictedSteps[frame] );
	}
}

/*
* CG_Predict
*/
void CG_Predict( void )
{
	int ucmdExecuted, ucmdHead;
	usercmd_t cmd;
	move_t move;
	gsplayerclass_t	*playerClass;
	cg_clientInfo_t *ci;

	ucmdHead = trap_NET_GetCurrentUserCmdNum();
	ucmdExecuted = cg.frame.ucmdExecuted;

	// clear the triggered toggles for this prediction round
	memset( &cg_triggersListTriggered, qfalse, sizeof( cg_triggersListTriggered ) );

	if( cg.predictFrom > 0 )
	{
		if( !cg_predict_optimize->integer || ( ucmdHead - cg.predictFrom >= CMD_BACKUP ) )
			cg.predictFrom = 0;
		else
		{
			ucmdExecuted = cg.predictFrom;
			cg.predictedPlayerState = cg.predictFromPlayerState;
			cg.predictedEntityState = cg.predictFromEntityState;
		}
	}

	// if we are too far out of date, just freeze
	if( ucmdHead - ucmdExecuted >= CMD_BACKUP )
	{
		if( cg_predict_debug->integer )
			GS_Printf( "exceeded CMD_BACKUP\n" );
		return;
	}

	// copy current state to move
	memset( &move, 0, sizeof( move ) );
	move.ms = &cg.predictedEntityState.ms;
	move.entNum = cgs.playerNum;
	move.contentmask = GS_ContentMaskForState( &cg.predictedEntityState );

	playerClass = GS_PlayerClassByIndex( cg.predictedEntityState.playerclass );
	move.specifics = playerClass->movespec;
	if( !move.specifics )
		move.specifics = &defaultPlayerMoveSpec;

	ci = &cgs.clientInfo[ cgs.playerNum ];

	assert( cg.predictedPlayerState.fov > 0.0f );

	while( ++ucmdExecuted <= ucmdHead )
	{
		if( ucmdExecuted == ucmdHead )
			trap_RefreshMouseAngles();

		trap_NET_GetUserCmd( ucmdExecuted & CMD_MASK, &cmd );

		predictingTimeStamp = cmd.serverTimeStamp;

		// apply user input to accel, weapon, button actions, etc
		GS_Client_ApplyUserInput( move.accel, &cg.predictedEntityState, &cg.predictedPlayerState, &cmd, ci->fov, ci->zoomfov, move.contentmask, 0 );

		// move it
		GS_SpaceUnLinkEntity( &cg_entities[cg.predictedEntityState.number].current );
		GS_Move( &move, cmd.msec, cg.predictedEntityState.local.boundmins, cg.predictedEntityState.local.boundmaxs );
		GS_SpaceLinkEntity( &cg_entities[cg.predictedEntityState.number].current );
		assert( cg.predictedPlayerState.fov > 0.0f );

		// Check for touching triggers
		CG_Predict_TouchTriggers( &move, cg.predictedEntityState.solid );

		if( cg.topPredictTimeStamp < predictingTimeStamp )
			cg.topPredictTimeStamp = predictingTimeStamp;

		// backup the last predicted ucmd which has a timestamp (it's closed)
		if( cg_predict_optimize->integer && ucmdExecuted == ucmdHead - 1 )
		{
			if( ucmdExecuted != cg.predictFrom )
			{
				cg.predictFrom = ucmdExecuted;
				cg.predictFromPlayerState = cg.predictedPlayerState;
				cg.predictFromEntityState = cg.predictedEntityState;
			}
		}

		// save some data
		predictedSteps[ucmdExecuted & CMD_MASK] = move.output.step; // copy for stair smoothing
		VectorCopy( move.ms->origin, cg.predictedOrigins[ucmdExecuted & CMD_MASK] ); // store for prediction error checks
	}

	// keep results
	cg.env = move.env;

	CG_PredictSmoothSteps();
}

#undef PREDICTED_STEP_TIME
