/*
Copyright (C) 2007 German Garcia
*/

#include "cg_local.h"

//==================================================
// SNAPSHOT UPDATE
//==================================================

/*
* CG_NewSnapLinkEntityState
*/
void CG_NewSnapLinkEntityState( const entity_state_t *state )
{
	centity_t *cent;

	assert( state->number >= 0 && state->number < MAX_EDICTS );

	cent = &cg_entities[state->number];

	// stuff to restart
	cent->noOldState = qfalse;
	cent->renderfx = 0;
	cent->effects = state->effects;
	cent->type = state->type;
	VectorClear( cent->avelocity );

	// event entities don't need processing
	if( ISEVENTENTITY( state ) )
	{
		cent->noOldState = qtrue;
		cent->canExtrapolate = cent->canExtrapolatePrev = qfalse;
		cent->snapNum = cg.frame.snapNum;
		cent->current = *state;
		// FIXME: local should not be inside state
		memset( &cent->current.local, 0, sizeof( cent->current.local ) );
		return;
	}

	if( state->ms.linearProjectileTimeStamp )
	{
		cent->noOldState = qtrue;
	}
	// if the entity wasn't present in the last frame, or changed drawing object
	else if( cent->snapNum != cg.oldFrame.snapNum
		|| state->modelindex1 != cent->current.modelindex1
		|| state->modelindex2 != cent->current.modelindex2
		|| state->solid != cent->current.solid
		|| state->cmodeltype != cent->current.cmodeltype )
	{
		cent->noOldState = qtrue;
	}

	cent->snapNum = cg.frame.snapNum;
	cent->prev = ( cent->noOldState ) ? *state : cent->current;
	cent->current = *state;
	cent->type = state->type;
	cent->effects = cent->current.effects;
	cent->renderfx = 0;
	cent->canExtrapolatePrev = ( cent->canExtrapolate && !cent->noOldState ) ? qtrue : qfalse;
	cent->canExtrapolate = ( cent->current.ms.linearProjectileTimeStamp == 0 ) ? qtrue : qfalse;
	cent->stopped = qfalse;

	if( cent->current.cmodeltype == CMODEL_BRUSH )
		cent->canExtrapolate = qfalse;

	// FIXME: local should not be inside state
	memset( &cent->current.local, 0, sizeof( cent->current.local ) );

	//if( ISEVENTENTITY( state ) )
	//	return;

	// update its local bounding box
	if( cent->current.solid )
	{
		if( cent->current.cmodeltype == CMODEL_BRUSH )
		{
			if( cent->current.modelindex1 )
			{
				struct cmodel_s	*cmodel = trap_CM_InlineModel( cent->current.modelindex1 );
				trap_CM_InlineModelBounds( cmodel, cent->current.local.mins, cent->current.local.maxs );
			}
		}
		else if( cent->current.cmodeltype != CMODEL_NOT )
		{
			GS_DecodeEntityBBox( cent->current.bbox, cent->current.local.mins, cent->current.local.maxs );
		}
	}

	// update velocities
	if( cent->noOldState )
	{
	}
	else
	{
		int i;

		// rotational velocity
		for( i = 0; i < 3; i++ )
			cent->avelocity[i] = AngleDelta( cent->current.ms.angles[i], cent->prev.ms.angles[i] );

		VectorScale( cent->avelocity, 1000.0f/cg.snap.frameTime, cent->avelocity );

		// if it moved too much force the teleported bit
		if(  abs( cent->current.ms.origin[0] - cent->prev.ms.origin[0] ) > 512
			|| abs( cent->current.ms.origin[1] - cent->prev.ms.origin[1] ) > 512
			|| abs( cent->current.ms.origin[2] - cent->prev.ms.origin[2] ) > 512 )
			cent->current.flags |= SFL_TELEPORTED;

		// if it was teleported put move on final position
		if( cent->current.flags & SFL_TELEPORTED )
		{
			VectorCopy( cent->current.ms.origin, cent->prev.ms.origin );
			VectorCopy( cent->current.ms.velocity, cent->prev.ms.velocity );
		}

		if( !VectorLengthFast( cent->prev.ms.velocity ) && !VectorLengthFast( cent->current.ms.velocity ) )
			cent->stopped = qtrue;
	}

	CG_Clip_LinkEntityState( &cent->current );
}

/*
* CG_NewSnapshot
*/
void CG_NewSnapshot( unsigned int snapNum, unsigned int serverTime )
{
	int prevSnapNum, i;

	if( snapNum <= 0 )
		GS_Error( "CG_NewSnapshot: Bad snapshot number %i\n", snapNum );

	cg.time = serverTime;
	cg.serverTime = serverTime;

	cg.oldAreabits = qfalse;
	cg.portalInView = qfalse;

	// if it's the first snap, use it for both prev and current states
	if( cg.frame.snapNum > 0 )
		prevSnapNum = cg.frame.snapNum;
	else
		prevSnapNum = snapNum;

	cg.frame = *trap_GetSnapshot( snapNum );
	cg.oldFrame = *trap_GetSnapshot( prevSnapNum );

	if( cg.frame.snapNum != cg.oldFrame.snapNum )
	{
		if( memcmp( cg.oldFrame.areabits, cg.frame.areabits, sizeof( cg.frame.areabits ) ) == 0 )
			cg.oldAreabits = qtrue;
	}

	cg.snap.newEntityEvents = qtrue;

	if( cg.oldFrame.timeStamp >= cg.frame.timeStamp )
		cg.snap.frameTime = 1;
	else
		cg.snap.frameTime = ( cg.frame.timeStamp - cg.oldFrame.timeStamp );

	if( !cg.snap.frameTime )
		cg.snap.frameTime = cgs.snapFrameTime;

	CG_RefreshTimeFracs();

	// update local gameState
	gs.gameState = *trap_GetGameStateFromSnapsBackup( cg.frame.snapNum );

	// update the local playerState
	cg.predictedPlayerState = *trap_GetPlayerStateFromSnapsBackup( cg.frame.snapNum );
	cg.predictedPlayerState.fov = trap_GetPlayerStateFromSnapsBackup( cg.oldFrame.snapNum )->fov;
	cg.predictedPlayerState.viewHeight = trap_GetPlayerStateFromSnapsBackup( cg.oldFrame.snapNum )->viewHeight;

	// start the new snapshot
	CG_Clip_UnlinkAllEntities();

	// can't use traces : re-links the entity states, and links them to the space partition
	for( i = 0; i < cg.frame.ents.numSnapshotEntities; i++ )
		CG_NewSnapLinkEntityState( trap_GetEntityFromSnapsBackup( cg.frame.snapNum, cg.frame.ents.snapshotEntities[i] ) );

	// can use traces : updates the visual info and other derived information
	CG_UpdateEntitiesState();

	// force interruption of prediction optimization so it's restarted
	cg.predictFrom = 0;
	CG_CheckPredictionError();
}
