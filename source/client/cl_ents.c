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
// cl_ents.c -- entity parsing and management

#include "client.h"

/*
   =========================================================================

   FRAME PARSING

   =========================================================================
 */

//=================
//CL_GetEntityFromSnapsBackup
//=================
static inline entity_state_t *CL_GetEntityFromSnapsBackup( unsigned int snapNum, int entNum )
{
	assert( entNum >= 0 && entNum < MAX_EDICTS );
	return &cl.snapsData.entityStateBackups[( snapNum & SNAPS_BACKUP_MASK )][entNum];
}

//=================
//CL_WriteDeltaEntity
//=================
void CL_WriteDeltaEntity( unsigned int entNum, unsigned int deltaSnapNum, unsigned int snapNum, msg_t *msg, qboolean force )
{
	entity_state_t *from, *to;

	assert( entNum >= 0 && entNum < MAX_EDICTS );

	if( !deltaSnapNum && !snapNum )
	{                             // write the baseline
		memset( &nullEntityState, 0, sizeof( nullEntityState ) );
		from = &nullEntityState;
		to = cl.snapsData.baselines + entNum;
	}
	else if( !deltaSnapNum )
	{                      // delta-write from baseline
		from = cl.snapsData.baselines + entNum;
		to = CL_GetEntityFromSnapsBackup( snapNum, entNum );
	}
	else
	{  // delta-write from snap
		from = CL_GetEntityFromSnapsBackup( deltaSnapNum, entNum );
		to = CL_GetEntityFromSnapsBackup( snapNum, entNum );
	}

	Com_WriteDeltaEntityState( from, to, msg, force );
}

//=================
//CL_ReadDeltaEntityState
//=================
void CL_ReadDeltaEntityState( msg_t *msg, int entNum, unsigned int deltaSnapNum, unsigned int snapNum, unsigned int bits )
{
	entity_state_t *from, *to;

	assert( entNum >= 0 && entNum < MAX_EDICTS );

	if( !deltaSnapNum && !snapNum )
	{
		memset( &nullEntityState, 0, sizeof( nullEntityState ) );
		from = &nullEntityState;
		to = cl.snapsData.baselines + entNum;
	}
	else if( !deltaSnapNum )
	{
		from = cl.snapsData.baselines + entNum;
		to = CL_GetEntityFromSnapsBackup( snapNum, entNum );
	}
	else
	{
		from = CL_GetEntityFromSnapsBackup( deltaSnapNum, entNum );
		to = CL_GetEntityFromSnapsBackup( snapNum, entNum );
	}

	Com_ReadDeltaEntityState( msg, from, to, entNum, bits );
}

//=================
//CL_GetGameStateFromSnapsBackup
//=================
static inline game_state_t *CL_GetGameStateFromSnapsBackup( unsigned int snapNum )
{
	assert( snapNum > 0 );
	return &cl.snapsData.gameStateBackups[( snapNum & SNAPS_BACKUP_MASK )];
}

//=================
//CL_ReadDeltaGameState
//=================
void CL_ReadDeltaGameState( msg_t *msg, unsigned int deltaSnapNum, unsigned int snapNum )
{
	game_state_t *gameState, *deltaGameState;

	gameState = CL_GetGameStateFromSnapsBackup( snapNum );
	if( !deltaSnapNum )
	{
		memset( &nullGameState, 0, sizeof( nullGameState ) );
		deltaGameState = &nullGameState;
	}
	else
	{
		deltaGameState = CL_GetGameStateFromSnapsBackup( deltaSnapNum );
	}

	Com_ReadDeltaGameState( msg, deltaGameState, gameState );
}

//=================
//CL_WriteDeltaGameState
//=================
void CL_WriteDeltaGameState( msg_t *msg, unsigned int deltaSnapNum, unsigned int snapNum )
{
	game_state_t *gameState, *deltaGameState;

	gameState = CL_GetGameStateFromSnapsBackup( snapNum );
	if( !deltaSnapNum )
	{
		memset( &nullGameState, 0, sizeof( nullGameState ) );
		deltaGameState = &nullGameState;
	}
	else
	{
		deltaGameState = CL_GetGameStateFromSnapsBackup( deltaSnapNum );
	}

	MSG_WriteByte( msg, svc_gameinfo );
	Com_WriteDeltaGameState( msg, deltaGameState, gameState );
}

//=================
//CL_GetPlayerStateFromSnapsBackup
//=================
static inline player_state_t *CL_GetPlayerStateFromSnapsBackup( unsigned int snapNum )
{
	assert( snapNum > 0 );
	return &cl.snapsData.playerStateBackups[( snapNum & SNAPS_BACKUP_MASK )];
}

//=================
//CL_ReadDeltaPlayerState
//=================
void CL_ReadDeltaPlayerState( msg_t *msg, unsigned int deltaSnapNum, unsigned int snapNum )
{
	player_state_t *playerState, *deltaPlayerState;

	playerState = CL_GetPlayerStateFromSnapsBackup( snapNum );
	if( !deltaSnapNum )
	{
		memset( &nullPlayerState, 0, sizeof( nullPlayerState ) );
		deltaPlayerState = &nullPlayerState;
	}
	else
	{
		deltaPlayerState = CL_GetPlayerStateFromSnapsBackup( deltaSnapNum );
	}

	Com_ReadDeltaPlayerState( msg, deltaPlayerState, playerState );
}

//=================
//CL_WriteDeltaPlayerState
//=================
void CL_WriteDeltaPlayerState( msg_t *msg, unsigned int deltaSnapNum, unsigned int snapNum )
{
	player_state_t *playerState, *deltaPlayerState;

	playerState = CL_GetPlayerStateFromSnapsBackup( snapNum );
	if( !deltaSnapNum )
	{
		memset( &nullPlayerState, 0, sizeof( nullPlayerState ) );
		deltaPlayerState = &nullPlayerState;
	}
	else
	{
		deltaPlayerState = CL_GetPlayerStateFromSnapsBackup( deltaSnapNum );
	}

	MSG_WriteByte( msg, svc_playerinfo );
	Com_WriteDeltaPlayerstate( msg, deltaPlayerState, playerState );
}

//=================
//CL_ParsePacketEntities
//=================
static void CL_ParsePacketEntities( msg_t *msg, snapshot_t *snap )
{
	int newnum;
	unsigned bits;
	int oldindex, oldnum;

	snap->ents.numSnapshotEntities = 0;
	oldindex = 0;
	if( !snap->deltaSnapNum || snap->snapFlags & SNAPFLAG_NODELTA )
	{
		oldnum = 99999;
	}
	else if( oldindex >= cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.numSnapshotEntities )
	{
		oldnum = 99999;
	}
	else
	{
		oldnum = cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.snapshotEntities[oldindex];
	}

	while( 1 )
	{
		newnum = Com_ReadEntityBits( msg, &bits );

		if( newnum == MAX_EDICTS )  // EOF
			break;

		if( newnum > MAX_EDICTS )
			Com_Error( ERR_DROP, "CL_ParsePacketEntities: bad number:%i", newnum );
		if( msg->readcount > msg->cursize )
			Com_Error( ERR_DROP, "CL_ParsePacketEntities: end of message" );

		// parse the entity_state_s data

		while( oldnum < newnum )
		{ // one or more entities from the old packet are unchanged
			if( cl_shownet->integer == 3 )
				Com_Printf( "   unchanged: %i\n", oldnum );

			// put it in the new list
			CL_ReadDeltaEntityState( msg, oldnum, snap->deltaSnapNum, snap->snapNum, 0 );
			snap->ents.snapshotEntities[snap->ents.numSnapshotEntities] = oldnum;
			snap->ents.numSnapshotEntities++;
			oldindex++;
			if( oldindex >= cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.numSnapshotEntities )
			{
				oldnum = 99999;
			}
			else
			{
				oldnum = cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.snapshotEntities[oldindex];
			}
		}

		// delta from baseline
		if( oldnum > newnum )
		{
			if( bits & U_HEADER_REMOVE )
			{
				Com_DPrintf( "U_REMOVE: oldnum > newnum (can't remove from baseline!)\n" );
				continue;
			}

			// delta from baseline
			if( cl_shownet->integer == 3 )
				Com_Printf( "   baseline: %i\n", newnum );

			// put it in the new list
			CL_ReadDeltaEntityState( msg, newnum, 0, snap->snapNum, bits );
			snap->ents.snapshotEntities[snap->ents.numSnapshotEntities] = newnum;
			snap->ents.numSnapshotEntities++;
			continue;
		}

		if( oldnum == newnum )
		{
			if( bits & U_HEADER_REMOVE )
			{ // the entity present in oldframe is not in the current frame
				if( cl_shownet->integer == 3 )
					Com_Printf( "   remove: %i\n", newnum );

				if( oldnum != newnum )
					Com_Printf( "U_REMOVE: oldnum != newnum\n" );

				oldindex++;
				if( oldindex >= cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.numSnapshotEntities )
				{
					oldnum = 99999;
				}
				else
				{
					oldnum = cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.snapshotEntities[oldindex];
				}

				continue;
			}

			// delta from previous state
			if( cl_shownet->integer == 3 )
				Com_Printf( "   delta: %i\n", newnum );

			// put it in the new list
			CL_ReadDeltaEntityState( msg, newnum, snap->deltaSnapNum, snap->snapNum, bits );
			snap->ents.snapshotEntities[snap->ents.numSnapshotEntities] = newnum;
			snap->ents.numSnapshotEntities++;
			oldindex++;
			if( oldindex >= cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.numSnapshotEntities )
			{
				oldnum = 99999;
			}
			else
			{
				oldnum = cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.snapshotEntities[oldindex];
			}
			continue;
		}
	}

	// any remaining entities in the old frame are copied over
	while( oldnum != 99999 )
	{ // one or more entities from the old packet are unchanged
		if( cl_shownet->integer == 3 )
			Com_Printf( "   unchanged: %i\n", oldnum );

		// put it in the new list
		CL_ReadDeltaEntityState( msg, oldnum, snap->deltaSnapNum, snap->snapNum, 0 );
		snap->ents.snapshotEntities[snap->ents.numSnapshotEntities] = oldnum;
		snap->ents.numSnapshotEntities++;
		oldindex++;
		if( oldindex >= cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.numSnapshotEntities )
		{
			oldnum = 99999;
		}
		else
		{
			oldnum = cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].ents.snapshotEntities[oldindex];
		}
	}
}

//=================
//CL_ParseSnapshot
//=================
static snapshot_t *CL_ParseSnapshot( msg_t *msg )
{
	int cmd, len;
	snapshot_t *snap;
	unsigned int snapNum, timeStamp;

	// read the header
	snapNum = MSG_ReadLong( msg );

	// clear up for reading
	snap = &cl.snapShots[snapNum & SNAPS_BACKUP_MASK];
	memset( snap, 0, sizeof( snapshot_t ) );

	snap->snapNum = snapNum;
	snap->snapFlags = (qbyte)MSG_ReadByte( msg );
	timeStamp = (unsigned int)MSG_ReadLong( msg );
	snap->timeStamp = timeStamp;
	snap->ucmdExecuted = MSG_ReadLong( msg );
	if( !( snap->snapFlags & SNAPFLAG_NODELTA ) )
		snap->deltaSnapNum = (unsigned int)MSG_ReadLong( msg );

	// validate the new snapshot
	if( snap->snapFlags & SNAPFLAG_NODELTA ) // not delta compressed
	{
		snap->valid = qtrue;
		if( cls.demo.waiting )
			cls.demo.basetime = snap->timeStamp;
		cls.demo.waiting = qfalse; // we can start recording now
	}
	// If the frame is delta compressed from data that we no longer have available,
	else if( cl.snapShots[snap->deltaSnapNum & SNAPS_BACKUP_MASK].snapNum != snap->deltaSnapNum )
	{
		snap->valid = qfalse;
		Com_Printf( "Delta snap too old.\n" );
	}
	else
	{
		snap->valid = qtrue;
	}

	// read areabits
	len = MSG_ReadByte( msg );
	if( len > sizeof( snap->areabits ) )
		Com_Error( ERR_DROP, "CL_ParseSnapshot: invalid areabits size" );
	memset( snap->areabits, 0, sizeof( snap->areabits ) );
	MSG_ReadData( msg, &snap->areabits, len );

	// read gameinfo
	cmd = MSG_ReadByte( msg );
	SHOWNET( msg, svc_strings[cmd] );
	if( cmd != svc_gameinfo )
		Com_Error( ERR_DROP, "CL_ParseSnapshot: not gameinfo" );
	CL_ReadDeltaGameState( msg, snap->deltaSnapNum, snap->snapNum );

	// read playerinfo
	cmd = MSG_ReadByte( msg );
	SHOWNET( msg, svc_strings[cmd] );
	if( cmd != svc_playerinfo )
		Com_Error( ERR_DROP, "CL_ParseSnapshot: not playerinfo" );
	CL_ReadDeltaPlayerState( msg, snap->deltaSnapNum, snap->snapNum );

	// read packet entities
	cmd = MSG_ReadByte( msg );
	SHOWNET( msg, svc_strings[cmd] );
	if( cmd != svc_packetentities )
		Com_Error( ERR_DROP, "CL_ParseFrame: not packetentities" );
	CL_ParsePacketEntities( msg, snap );

	return snap;
}

//=================
//CL_ParseFrame - Parses the frame and stores the data in snap backups, but doesn't fire the new snap
//=================
void CL_ParseFrame( msg_t *msg )
{
	snapshot_t *snap;

	snap = CL_ParseSnapshot( msg );

	if( snap->valid )
	{
		cl.receivedSnapNum = snap->snapNum;

		if( cls.demo.recording )
			cls.demo.duration = snap->timeStamp - cls.demo.basetime;

#ifdef SMOOTHSERVERTIME
		if( !cls.demo.playing )
		{
			// the first snap, fill all the timeDeltas with the same value
			if( cl.currentSnapNum <= 0 || ( cl.serverTime + 250 < snap->timeStamp ) )
			{
				int i;

				cl.serverTimeDelta = cl.newServerTimeDelta = snap->timeStamp - cls.gametime - 1;
				for( i = 0; i < MAX_TIMEDELTAS_BACKUP; i++ )
					cl.serverTimeDeltas[i] = cl.newServerTimeDelta;

				if( cl_debug_timeDelta->integer )
					Com_Printf( S_COLOR_CYAN"***** timeDelta restarted\n" );
			}
			else
			{
				int delta;

				// don't let delta add big jumps to the smoothing ( a stable connection produces jumps inside +-3 range)
				delta = ( snap->timeStamp - cl.snapFrameTime ) - cls.gametime;
				if( delta < cl.serverTimeDelta - 150 || delta > cl.serverTimeDelta + 150 )
				{
					int i;

					cl.serverTimeDelta = cl.newServerTimeDelta = delta;
					for( i = 0; i < MAX_TIMEDELTAS_BACKUP; i++ )
						cl.serverTimeDeltas[i] = delta;

					if( cl_debug_timeDelta->integer )
						Com_Printf( S_COLOR_CYAN"***** timeDelta jumped\n" );
				}
				else
				{
					if( cl_debug_timeDelta->integer )
					{
						if( delta < cl.serverTimeDelta - (int)cl.snapFrameTime )
							Com_Printf( S_COLOR_CYAN"***** timeDelta low clamp\n" );
						else if( delta > cl.serverTimeDelta + (int)cl.snapFrameTime )
							Com_Printf( S_COLOR_CYAN"***** timeDelta high clamp\n" );
					}

					clamp( delta, cl.serverTimeDelta - (int)cl.snapFrameTime, cl.serverTimeDelta + (int)cl.snapFrameTime );

					cl.serverTimeDeltas[cl.receivedSnapNum & MASK_TIMEDELTAS_BACKUP] = delta;
				}
			}
		}
#endif
	}
}
