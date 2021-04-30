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

#include "server.h"

void SV_CreateBaselines( void )
{
	entity_state_t *state;
	int i;

	memset( sv.snapsData.baselines, 0, sizeof( entity_state_t )*MAX_EDICTS );
	for( i = 0; i < MAX_EDICTS; i++ )
	{
		state = ge->GetEntityState( i );

		if( Com_EntityIsBaseLined( state ) )
			sv.snapsData.baselines[i] = *state;

		sv.snapsData.baselines[i].number = i;
	}
}

void SV_BackUpSnapshotData( unsigned int snapNum, int numEntities )
{
	int i;
	entity_state_t *entityBackups, *entityState;
	player_state_t *playerBackups, *playerState;
	game_state_t *gameBackup;

	entityBackups = &sv.snapsData.entityStateBackups[( snapNum & SNAPS_BACKUP_MASK )][0];
	playerBackups = &sv.snapsData.playerStateBackups[( snapNum & SNAPS_BACKUP_MASK )][0];

	memset( entityBackups, 0, sizeof( entity_state_t ) * MAX_EDICTS );
	memset( playerBackups, 0, sizeof( player_state_t ) * MAX_CLIENTS );

	gameBackup = &sv.snapsData.gameStateBackups[( snapNum & SNAPS_BACKUP_MASK )];
	*gameBackup = *ge->GetGameState();
	
	for( i = 0; i < numEntities; i++ )
	{
		entityState = entityBackups + i;
		*entityState = *ge->GetEntityState( i );
	}

	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state < CS_SPAWNED )
			continue;

		playerState = playerBackups + i;
		*playerState = *ge->GetPlayerState( i );
	}
}

//=================
//SV_GetEntityFromSnapsBackup
//=================
static inline entity_state_t *SV_GetEntityFromSnapsBackup( unsigned int snapNum, int entNum )
{
	assert( entNum >= 0 && entNum < MAX_EDICTS );
	return &sv.snapsData.entityStateBackups[( snapNum & SNAPS_BACKUP_MASK )][entNum];
}

//=================
//SV_WriteDeltaEntity
//=================
void SV_WriteDeltaEntity( msg_t *msg, unsigned int entNum, unsigned int deltaSnapNum, unsigned int snapNum, qboolean force )
{
	entity_state_t *from, *to;

	assert( entNum >= 0 && entNum < MAX_EDICTS );

	if( !deltaSnapNum && !snapNum )
	{                             // write the baseline
		memset( &nullEntityState, 0, sizeof( nullEntityState ) );
		from = &nullEntityState;
		to = sv.snapsData.baselines + entNum;
	}
	else if( !deltaSnapNum )
	{                      // delta-write from baseline
		from = sv.snapsData.baselines + entNum;
		to = SV_GetEntityFromSnapsBackup( snapNum, entNum );
	}
	else
	{  // delta-write from snap
		from = SV_GetEntityFromSnapsBackup( deltaSnapNum, entNum );
		to = SV_GetEntityFromSnapsBackup( snapNum, entNum );
	}

	Com_WriteDeltaEntityState( from, to, msg, force );
}

//=================
//SV_ReadDeltaEntityState
//=================
void SV_ReadDeltaEntityState( msg_t *msg, int entNum, unsigned int deltaSnapNum, unsigned int snapNum, unsigned int bits )
{
	entity_state_t *from, *to;

	assert( entNum >= 0 && entNum < MAX_EDICTS );

	if( !deltaSnapNum && !snapNum )
	{
		memset( &nullEntityState, 0, sizeof( nullEntityState ) );
		from = &nullEntityState;
		to = sv.snapsData.baselines + entNum;
	}
	else if( !deltaSnapNum )
	{
		from = sv.snapsData.baselines + entNum;
		to = SV_GetEntityFromSnapsBackup( snapNum, entNum );
	}
	else
	{
		from = SV_GetEntityFromSnapsBackup( deltaSnapNum, entNum );
		to = SV_GetEntityFromSnapsBackup( snapNum, entNum );
	}

	Com_ReadDeltaEntityState( msg, from, to, entNum, bits );
}

//=================
//SV_GetGameStateFromSnapsBackup
//=================
static inline game_state_t *SV_GetGameStateFromSnapsBackup( unsigned int snapNum )
{
	return &sv.snapsData.gameStateBackups[( snapNum & SNAPS_BACKUP_MASK )];
}

//=================
//SV_ReadDeltaGameState
//=================
void SV_ReadDeltaGameState( msg_t *msg, unsigned int deltaSnapNum, unsigned int snapNum )
{
	game_state_t *gameState, *deltaGameState;

	gameState = SV_GetGameStateFromSnapsBackup( snapNum );
	if( !deltaSnapNum )
	{
		memset( &nullGameState, 0, sizeof( nullGameState ) );
		deltaGameState = &nullGameState;
	}
	else
	{
		deltaGameState = SV_GetGameStateFromSnapsBackup( deltaSnapNum );
	}

	Com_ReadDeltaGameState( msg, deltaGameState, gameState );
}

//=================
//SV_WriteDeltaGameState
//=================
void SV_WriteDeltaGameState( msg_t *msg, unsigned int deltaSnapNum, unsigned int snapNum )
{
	game_state_t *gameState, *deltaGameState;

	gameState = SV_GetGameStateFromSnapsBackup( snapNum );
	if( !deltaSnapNum )
	{
		memset( &nullGameState, 0, sizeof( nullGameState ) );
		deltaGameState = &nullGameState;
	}
	else
	{
		deltaGameState = SV_GetGameStateFromSnapsBackup( deltaSnapNum );
	}

	MSG_WriteByte( msg, svc_gameinfo );
	Com_WriteDeltaGameState( msg, deltaGameState, gameState );
}

//=================
//SV_GetPlayerStateFromSnapsBackup
//=================
static inline player_state_t *SV_GetPlayerStateFromSnapsBackup( unsigned int snapNum, int clientNum )
{
	return &sv.snapsData.playerStateBackups[( snapNum & SNAPS_BACKUP_MASK )][clientNum];
}

//=================
//SV_ReadDeltaPlayerState
//=================
void SV_ReadDeltaPlayerState( msg_t *msg, int clientNum, unsigned int deltaSnapNum, unsigned int snapNum )
{
	player_state_t *playerState, *deltaPlayerState;

	playerState = SV_GetPlayerStateFromSnapsBackup( snapNum, clientNum );
	if( !deltaSnapNum )
	{
		memset( &nullPlayerState, 0, sizeof( nullPlayerState ) );
		deltaPlayerState = &nullPlayerState;
	}
	else
	{
		deltaPlayerState = SV_GetPlayerStateFromSnapsBackup( deltaSnapNum, clientNum );
	}

	Com_ReadDeltaPlayerState( msg, deltaPlayerState, playerState );
}

//=================
//SV_WriteDeltaPlayerState
//=================
void SV_WriteDeltaPlayerState( msg_t *msg, int clientNum, unsigned int deltaSnapNum, unsigned int snapNum )
{
	player_state_t *playerState, *deltaPlayerState;

	playerState = SV_GetPlayerStateFromSnapsBackup( snapNum, clientNum );
	if( !deltaSnapNum )
	{
		memset( &nullPlayerState, 0, sizeof( nullPlayerState ) );
		deltaPlayerState = &nullPlayerState;
	}
	else
	{
		deltaPlayerState = SV_GetPlayerStateFromSnapsBackup( deltaSnapNum, clientNum );
	}

	MSG_WriteByte( msg, svc_playerinfo );
	Com_WriteDeltaPlayerstate( msg, deltaPlayerState, playerState );
}

//=============================================================================
//
//Encode a client frame onto the network channel
//
//=============================================================================

//=============
//SV_EmitPacketEntities
//
//Writes a delta update of an entity_state_t list to the message.
//=============
static void SV_EmitPacketEntities( client_t *client, unsigned int deltaSnapNum, unsigned int snapNum, msg_t *msg )
{
	int oldindex, newindex;
	int oldnum, newnum;
	int from_num_entities;
	int bits;
	snapshotEntityNumbers_t *from, *to;

	MSG_WriteByte( msg, svc_packetentities );

	to = &client->snaps[(snapNum & SNAPS_BACKUP_MASK)].ents;
	if( !deltaSnapNum )
	{
		from = NULL;
		from_num_entities = 0;
	}
	else
	{
		from = &client->snaps[(deltaSnapNum & SNAPS_BACKUP_MASK)].ents;
		from_num_entities = from->numSnapshotEntities;
	}

	newindex = 0;
	oldindex = 0;
	while( newindex < to->numSnapshotEntities || oldindex < from_num_entities )
	{
		if( newindex >= to->numSnapshotEntities )
		{
			newnum = 9999;
		}
		else
		{
			newnum = to->snapshotEntities[newindex];
		}

		if( oldindex >= from_num_entities )
		{
			oldnum = 9999;
		}
		else
		{
			oldnum = from->snapshotEntities[oldindex];
		}

		if( newnum == oldnum )
		{ // delta update from old position
			SV_WriteDeltaEntity( msg, newnum, deltaSnapNum, snapNum, qfalse );
			//ge->WriteDeltaEntity( deltaSnapNum, sv.framenum, newnum, msg, qfalse );
			oldindex++;
			newindex++;
			continue;
		}

		if( newnum < oldnum )
		{ // this is a new entity, send it from the baseline
			SV_WriteDeltaEntity( msg, newnum, 0, snapNum, qtrue );
			//ge->WriteDeltaEntity( 0, sv.framenum, newnum, msg, qtrue );
			newindex++;
			continue;
		}

		if( newnum > oldnum ) // the old entity isn't present in the new message
		{
			bits = U_HEADER_REMOVE;
			if( oldnum >= 256 )
				bits |= ( U_HEADER_NUMBER16 | U_HEADER_MOREBITS1 );

			MSG_WriteByte( msg, bits&0xFF );
			if( bits & 0x0000FF00 )
				MSG_WriteByte( msg, ( bits>>8 )&0xFF );

			if( bits & U_HEADER_NUMBER16 )
				MSG_WriteShort( msg, oldnum );
			else
				MSG_WriteByte( msg, oldnum );

			oldindex++;
			continue;
		}
	}

	// End of packet entities
	MSG_WriteByte( msg, U_HEADER_MOREBITS1 );
	MSG_WriteByte( msg, ( U_HEADER_NUMBER16>>8 )&0xFF );
	MSG_WriteShort( msg, MAX_EDICTS );
}

//==================
//SV_WriteFrameSnapToClient
//==================
void SV_WriteFrameSnapToClient( client_t *client, msg_t *msg )
{
	snapshot_t *snap;

	snap = &client->snaps[sv.framenum & SNAPS_BACKUP_MASK];
	if( snap->snapNum != sv.framenum )
		Com_Error( ERR_FATAL, "SV_WriteFrameSnapToClient: Attempting to write a snap different than current" );

	if( client->noDeltaSnap )
		snap->deltaSnapNum = 0;
	else
		snap->deltaSnapNum = client->snapAcknowledged;

	if( !snap->deltaSnapNum || ( snap->deltaSnapNum > sv.framenum )
	   || ( client->snaps[snap->deltaSnapNum & SNAPS_BACKUP_MASK].snapNum != snap->deltaSnapNum ) )
	{
		snap->snapFlags |= SNAPFLAG_NODELTA;
		snap->deltaSnapNum = 0;
	}

	snap->timeStamp = svs.gametime;
	snap->ucmdExecuted = client->UcmdExecuted;

	MSG_WriteByte( msg, svc_frame );
	MSG_WriteLong( msg, snap->snapNum );
	MSG_WriteByte( msg, snap->snapFlags );
	MSG_WriteLong( msg, snap->timeStamp );
	MSG_WriteLong( msg, snap->ucmdExecuted );
	if( !( snap->snapFlags & SNAPFLAG_NODELTA ) )
		MSG_WriteLong( msg, snap->deltaSnapNum );

	// send over the areabits
	MSG_WriteByte( msg, snap->areabytes );
	MSG_WriteData( msg, snap->areabits, snap->areabytes );

	SV_WriteDeltaGameState( msg, snap->deltaSnapNum, snap->snapNum );

	// delta encode the playerstate
	SV_WriteDeltaPlayerState( msg, CLIENTNUM( client ), snap->deltaSnapNum, snap->snapNum );

	// delta encode the entities
	SV_EmitPacketEntities( client, snap->deltaSnapNum, snap->snapNum, msg );
}

/*
   =============================================================================

   Build a client frame structure

   =============================================================================
 */
#if 0
/*
* CM_SetPVS
*/
void CM_SetPVS( vec3_t origin, qbyte *pvs, qboolean merge )
{
	if( !merge )
		memset( pvs, 0, CM_ClusterSize() );
	CM_MergePVS( origin, pvs );
}

/*
* CM_SetPHS
*/
void CM_SetPHS( vec3_t origin, qbyte *phs, qboolean merge )
{
	if( !merge )
		memset( phs, 0, CM_ClusterSize() );
	CM_MergePHS( CM_LeafCluster( CM_PointLeafnum( origin ) ), phs );
}

/*
* CM_SetArea
*/
int CM_SetArea( vec3_t origin, qbyte *areabits, qboolean merge )
{
	int area = CM_LeafArea( CM_PointLeafnum( origin ) );

	if( merge )
		return CM_MergeAreaBits( areabits, area );

	return CM_WriteAreaBits( areabits, area );
}
#endif

/*
* CM_BitsCull
*/
qboolean CM_BitsCull( int headnode, int num_clusters, int *clusternums, qbyte *bits, int max_clusters )
{
	int i, l;

	// too many leafs for individual check, go by headnode
	if( num_clusters == -1 || !clusternums )
	{
		if( !CM_HeadnodeVisible( svs.cms, headnode, bits ) )
			return qtrue;
		return qfalse;
	}

	// check individual leafs
	for( i = 0; i < max_clusters; i++ )
	{
		l = clusternums[i];
		if( bits[l >> 3] & ( 1 << ( l&7 ) ) )
			return qfalse;
	}

	return qtrue;	// not visible/audible
}

/*
* CM_AreaCullBits
*/
qboolean CM_AreaCullBits( qbyte *areabits, int areanum, int areanum2 )
{
	// this test includes the merged portal areas
	if( areanum > -1 && !( areabits[areanum>>3] & ( 1<<( areanum&7 ) ) ) )
	{
		// doors can legally straddle two areas, so we may need to check another one
		if( areanum2 <= -1 || !( areabits[areanum2>>3] & ( 1<<( areanum2&7 ) ) ) )
			return qtrue; // blocked by a door
	}

	return qfalse;
}


typedef struct
{
	qbyte pvs[MAX_MAP_LEAFS/8];
	qbyte phs[MAX_MAP_LEAFS/8];
	qbyte areabits[MAX_MAP_AREAS/8];
	int areabytes;
} vis_set_t;

static vis_set_t vis;

/*
* SV_SetVIS
*/
void SV_SetVIS( vec3_t origin, qboolean merge )
{
	if( !merge )
		memset( &vis, 0, sizeof( vis_set_t ) );

	vis.areabytes = CM_MergeVisSets( svs.cms, origin, vis.pvs, vis.phs, vis.areabits );
}

/*
* SV_PVSCullEntity
*/
qboolean SV_PVSCullEntity( int ent_headnode, int ent_num_clusters, int *ent_clusternums )
{
	return CM_BitsCull( ent_headnode, ent_num_clusters, ent_clusternums, vis.pvs, ent_num_clusters );
}

/*
* SV_PHSCullEntity
*/
qboolean SV_PHSCullEntity( int ent_headnode, int ent_num_clusters, int *ent_clusternums )
{
	return CM_BitsCull( ent_headnode, ent_num_clusters, ent_clusternums, vis.phs, 1 );
}

/*
* SV_AreaCullEntity
*/
qboolean SV_AreaCullEntity( int ent_areanum, int ent_areanum2 )
{
	return CM_AreaCullBits( vis.areabits, ent_areanum, ent_areanum2 );
}

//=====================================================================

/*
* SV_SortEntitiesList
*/
static void SV_SortEntitiesList( snapshot_t *snap )
{
	qboolean addedToEntsList[MAX_EDICTS];
	int i, j;

	if( snap->ents.numSnapshotEntities < 2 )
		return;

	// entities list must be sorted in ascending order
	memset( addedToEntsList, qfalse, sizeof( addedToEntsList ) );
	for( i = 0, j = 0; i < snap->ents.numSnapshotEntities; i++ )
	{
		addedToEntsList[snap->ents.snapshotEntities[i]] = qtrue;
		if( snap->ents.snapshotEntities[i] > j )
			j = snap->ents.snapshotEntities[i];
	}

	snap->ents.numSnapshotEntities = 0;
	for( i = 0; i <= j; i++ )
	{
		if( addedToEntsList[i] )
		{
			snap->ents.snapshotEntities[snap->ents.numSnapshotEntities] = i;
			snap->ents.numSnapshotEntities++;
		}
	}
}

/*
* SV_BuildClientFrameSnap
* Decides which entities are going to be visible to the client, and
* copies off the player state and areabits.
*/
void SV_BuildClientFrameSnap( client_t *client )
{
	snapshot_t *snap;

	if( client->state < CS_SPAWNED )
		return;

	// build up the list of visible entities
	snap = &client->snaps[sv.framenum & SNAPS_BACKUP_MASK];
	memset( snap, 0, sizeof( snapshot_t ) );

	// let the game code create the list
	if( sv_snap_nocull->integer )
		ge->BuildSnapEntitiesList( CLIENTNUM( client ), &snap->ents, qfalse );
	else
		ge->BuildSnapEntitiesList( CLIENTNUM( client ), &snap->ents, qtrue );

	// validate the list count provided by the game code
	if( snap->ents.numSnapshotEntities >= MAX_SNAPSHOT_ENTITIES )
		Com_Error( ERR_FATAL, "SV_BuildClientFrameSnap: numSnapshotEntities >= MAX_SNAPSHOT_ENTITIES\n" );

	SV_SortEntitiesList( snap );

	// do not let the game code set these values
	snap->snapNum = sv.framenum;
	snap->timeStamp = svs.gametime;
	snap->ucmdExecuted = client->UcmdExecuted;

	// copy the frame vis info (created by ge->BuildSnapEntitiesList)
	snap->areabytes = vis.areabytes;
	memcpy( &snap->areabits, vis.areabits, sizeof( snap->areabits ) );
}
