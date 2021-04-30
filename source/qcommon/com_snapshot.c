
#include "qcommon.h"

entity_state_t nullEntityState;
player_state_t nullPlayerState;
game_state_t nullGameState;

//==================================================
// game_state_t communication
//==================================================

void Com_WriteDeltaGameState( msg_t *msg, game_state_t *deltaGameState, game_state_t *gameState )
{
	int i;
	short statbits;
	qbyte bits;

	assert( MAX_GAME_STATS == 16 );
	assert( MAX_GAME_LONGSTATS == 8 );

	bits = 0;
	for( i = 0; i < MAX_GAME_LONGSTATS; i++ )
	{
		if( deltaGameState->longstats[i] != gameState->longstats[i] )
			bits |= 1<<i;
	}

	statbits = 0;
	for( i = 0; i < MAX_GAME_STATS; i++ )
	{
		if( deltaGameState->stats[i] != gameState->stats[i] )
			statbits |= 1<<i;
	}

	MSG_WriteByte( msg, bits );
	MSG_WriteShort( msg, statbits );

	if( bits )
	{
		for( i = 0; i < MAX_GAME_LONGSTATS; i++ )
		{
			if( bits & ( 1<<i ) )
				MSG_WriteLong( msg, (int)gameState->longstats[i] );
		}
	}

	if( statbits )
	{
		for( i = 0; i < MAX_GAME_STATS; i++ )
		{
			if( statbits & ( 1<<i ) )
				MSG_WriteShort( msg, gameState->stats[i] );
		}
	}
}

void Com_ReadDeltaGameState( msg_t *msg, game_state_t *deltaGameState, game_state_t *gameState )
{
	short statbits;
	qbyte bits;
	int i;

	assert( MAX_GAME_STATS == 16 );
	assert( MAX_GAME_LONGSTATS == 8 );

	memcpy( gameState, deltaGameState, sizeof( game_state_t ) );

	bits = (qbyte)MSG_ReadByte( msg );
	statbits = MSG_ReadShort( msg );

	if( bits )
	{
		for( i = 0; i < MAX_GAME_LONGSTATS; i++ )
		{
			if( bits & ( 1<<i ) )
				gameState->longstats[i] = (unsigned int)MSG_ReadLong( msg );
		}
	}

	if( statbits )
	{
		for( i = 0; i < MAX_GAME_STATS; i++ )
		{
			if( statbits & ( 1<<i ) )
				gameState->stats[i] = MSG_ReadShort( msg );
		}
	}
}

//==================================================
// move_state_t communication
//==================================================

#define	FLMS_TYPE	    ( 1<<0 )
#define	FLMS_ORIGIN0	    ( 1<<1 )
#define	FLMS_ORIGIN1	    ( 1<<2 )
#define	FLMS_ORIGIN2	    ( 1<<3 )
#define	FLMS_ANGLES0	    ( 1<<4 )
#define	FLMS_ANGLES1	    ( 1<<5 )
#define	FLMS_ANGLES2	    ( 1<<6 )
#define FLMS_MOREBITS1	    ( 1<<7 )

#define	FLMS_VELOCITY0	    ( 1<<8 )
#define	FLMS_VELOCITY1	    ( 1<<9 )
#define	FLMS_VELOCITY2	    ( 1<<10 )
#define	FLMS_LINEARSTAMP    ( 1<<11 )
//...
#define FLMS_MOREBITS2	    ( 1<<15 )

unsigned int Com_BitmaskDeltaMoveState( move_state_t *deltaMoveState, move_state_t *moveState )
{
	unsigned int bitflags;

	bitflags = 0;

	// 1st byte
	if( deltaMoveState->type != moveState->type )
	{
		bitflags |= FLMS_TYPE;
	}

	if( !moveState->linearProjectileTimeStamp )
	{
		if( deltaMoveState->origin[0] != moveState->origin[0] )
		{
			bitflags |= FLMS_ORIGIN0;
		}
		if( deltaMoveState->origin[1] != moveState->origin[1] )
		{
			bitflags |= FLMS_ORIGIN1;
		}
		if( deltaMoveState->origin[2] != moveState->origin[2] )
		{
			bitflags |= FLMS_ORIGIN2;
		}
	}
	if( deltaMoveState->angles[0] != moveState->angles[0] )
	{
		bitflags |= FLMS_ANGLES0;
	}
	if( deltaMoveState->angles[1] != moveState->angles[1] )
	{
		bitflags |= FLMS_ANGLES1;
	}
	if( deltaMoveState->angles[2] != moveState->angles[2] )
	{
		bitflags |= FLMS_ANGLES2;
	}

	// 2nd byte
	if( deltaMoveState->velocity[0] != moveState->velocity[0] )
	{
		bitflags |= FLMS_VELOCITY0;
	}
	if( deltaMoveState->velocity[1] != moveState->velocity[1] )
	{
		bitflags |= FLMS_VELOCITY1;
	}
	if( deltaMoveState->velocity[2] != moveState->velocity[2] )
	{
		bitflags |= FLMS_VELOCITY2;
	}

	if( deltaMoveState->linearProjectileTimeStamp != moveState->linearProjectileTimeStamp )
	{
		bitflags |= FLMS_LINEARSTAMP;
	}

	// flags
	if( bitflags & 0x0000ff00 )
		bitflags |= FLMS_MOREBITS1;

	return bitflags;
}

unsigned int Com_WriteDeltaMoveState( msg_t *msg, move_state_t *deltaMoveState, move_state_t *moveState )
{
	unsigned int bitflags;

	bitflags = Com_BitmaskDeltaMoveState( deltaMoveState, moveState );

	// write the flags
	MSG_WriteByte( msg, bitflags & 0xff );
	if( bitflags & FLMS_MOREBITS1 )
	{
		MSG_WriteByte( msg, ( bitflags>>8 ) & 0xff );
	}

	// write the actual data
	if( bitflags & FLMS_TYPE )
		MSG_WriteByte( msg, moveState->type );

	if( bitflags & FLMS_ORIGIN0 )
		MSG_WriteInt3( msg, (int)( moveState->origin[0]*PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_ORIGIN1 )
		MSG_WriteInt3( msg, (int)( moveState->origin[1]*PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_ORIGIN2 )
		MSG_WriteInt3( msg, (int)( moveState->origin[2]*PM_VECTOR_SNAP ) );

	// angles will depend on the entity type. players and brush-models will send shorts, but other entities bytes.
	if( bitflags & FLMS_ANGLES0 )
		MSG_WriteAngle16( msg, moveState->angles[0] );
	if( bitflags & FLMS_ANGLES1 )
		MSG_WriteAngle16( msg, moveState->angles[1] );
	if( bitflags & FLMS_ANGLES2 )
		MSG_WriteAngle16( msg, moveState->angles[2] );

	if( bitflags & FLMS_VELOCITY0 )
		MSG_WriteInt3( msg, (int)( moveState->velocity[0]*PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_VELOCITY1 )
		MSG_WriteInt3( msg, (int)( moveState->velocity[1]*PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_VELOCITY2 )
		MSG_WriteInt3( msg, (int)( moveState->velocity[2]*PM_VECTOR_SNAP ) );

	if( bitflags & FLMS_LINEARSTAMP )
		MSG_WriteLong( msg, moveState->linearProjectileTimeStamp );

	return bitflags;
}

void Com_ReadDeltaMoveState( msg_t *msg, move_state_t *deltaMoveState, move_state_t *moveState )
{
	int bitflags, i;

	bitflags = MSG_ReadByte( msg );
	if( bitflags & FLMS_MOREBITS1 )
	{
		i = MSG_ReadByte( msg );
		bitflags |= ( i<<8 )&0x0000FF00;
	}

	// use old state as base
	memcpy( moveState, deltaMoveState, sizeof( move_state_t ) );

	// start reading the new data
	if( bitflags & FLMS_TYPE )
		moveState->type = MSG_ReadByte( msg );

	if( bitflags & FLMS_ORIGIN0 )
		moveState->origin[0] = ( (float)MSG_ReadInt3( msg )*( 1.0/PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_ORIGIN1 )
		moveState->origin[1] = ( (float)MSG_ReadInt3( msg )*( 1.0/PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_ORIGIN2 )
		moveState->origin[2] = ( (float)MSG_ReadInt3( msg )*( 1.0/PM_VECTOR_SNAP ) );

	if( bitflags & FLMS_ANGLES0 )
		moveState->angles[0] = MSG_ReadAngle16( msg );
	if( bitflags & FLMS_ANGLES1 )
		moveState->angles[1] = MSG_ReadAngle16( msg );
	if( bitflags & FLMS_ANGLES2 )
		moveState->angles[2] = MSG_ReadAngle16( msg );

	if( bitflags & FLMS_VELOCITY0 )
		moveState->velocity[0] = ( (float)MSG_ReadInt3( msg )*( 1.0/PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_VELOCITY1 )
		moveState->velocity[1] = ( (float)MSG_ReadInt3( msg )*( 1.0/PM_VECTOR_SNAP ) );
	if( bitflags & FLMS_VELOCITY2 )
		moveState->velocity[2] = ( (float)MSG_ReadInt3( msg )*( 1.0/PM_VECTOR_SNAP ) );

	if( bitflags & FLMS_LINEARSTAMP )
		moveState->linearProjectileTimeStamp = (unsigned int)MSG_ReadLong( msg );

	// reference for when I want to add lods
	//if( bits & FLES_ANGLE1 && ((to->cmodeltype == CMODEL_BRUSH)||(to->cmodeltype == CMODEL_BBOX_ROTATED)) )
	//	MSG_WriteAngle16( msg, to->angles[0] );
	//else if( bits & FLES_ANGLE1 )
	//	MSG_WriteAngle( msg, to->angles[0] );
}

//==================================================
// player_state_t communication
//==================================================

#define	FLPS_WEAPONSTATE	( 1<<0 )
#define	FLPS_EVENT	    ( 1<<1 )
#define	FLPS_EVENT2	    ( 1<<2 )
#define FLPS_MOVETIMERS	    ( 1<<3 )
#define FLPS_HOLDABLELIST	( 1<<4 )
#define	FLPS_VIEWHEIGHT	    ( 1<<5 )
//....
#define FLPS_MOREBITS1	    ( 1<<7 )


#define	FLPS_DELTA_ANGLES0  ( 1<<8 )
#define	FLPS_DELTA_ANGLES1  ( 1<<9 )
#define	FLPS_DELTA_ANGLES2  ( 1<<10 )
//....
#define FLPS_MOREBITS2	    ( 1<<15 )


#define	FLPS_POVNUM	    ( 1<<16 )
#define	FLPS_FOV	    ( 1<<17 )
#define	FLPS_VIEWTYPE	    ( 1<<18 )
//....
#define FLPS_MOREBITS3	    ( 1<<23 )

//=================
//Com_WriteDeltaPlayerstate
//=================
void Com_WriteDeltaPlayerstate( msg_t *msg, player_state_t *deltaPlayerState, player_state_t *playerState )
{
	int i;
	int pflags;
	int statbits;

	pflags = 0;

	if( playerState->weaponState != deltaPlayerState->weaponState )
		pflags |= FLPS_WEAPONSTATE;

	if( playerState->delta_angles[0] != deltaPlayerState->delta_angles[0] )
		pflags |= FLPS_DELTA_ANGLES0;
	if( playerState->delta_angles[1] != deltaPlayerState->delta_angles[1] )
		pflags |= FLPS_DELTA_ANGLES1;
	if( playerState->delta_angles[2] != deltaPlayerState->delta_angles[2] )
		pflags |= FLPS_DELTA_ANGLES2;

	for( i = 0; i < MAX_USERINPUT_STATS; i++ )
		if( playerState->controlTimers[i] != deltaPlayerState->controlTimers[i] )
			pflags |= FLPS_MOVETIMERS;

	for( i = 0; i < INV_MAX_SLOTS; i++ )
	{
		if( playerState->inventory[i][0] != deltaPlayerState->inventory[i][0] ||
		    playerState->inventory[i][1] != deltaPlayerState->inventory[i][1] )
		{
			pflags |= FLPS_HOLDABLELIST;
			break;
		}
	}

	if( playerState->event[0] )
		pflags |= FLPS_EVENT;

	if( playerState->event[1] )
		pflags |= FLPS_EVENT2;

	if( playerState->POVnum != deltaPlayerState->POVnum )
		pflags |= FLPS_POVNUM;

	if( playerState->viewHeight != deltaPlayerState->viewHeight )
		pflags |= FLPS_VIEWHEIGHT;

	if( playerState->fov != deltaPlayerState->fov )
		pflags |= FLPS_FOV;

	if( playerState->viewType != deltaPlayerState->viewType )
		pflags |= FLPS_VIEWTYPE;

	//------- write it ---------

	if( pflags & 0xff000000 )
		pflags |= FLPS_MOREBITS3 | FLPS_MOREBITS2 | FLPS_MOREBITS1;
	else if( pflags & 0x00ff0000 )
		pflags |= FLPS_MOREBITS2 | FLPS_MOREBITS1;
	else if( pflags & 0x0000ff00 )
		pflags |= FLPS_MOREBITS1;

	MSG_WriteByte( msg, pflags&255 );

	if( pflags & 0xff000000 )
	{
		MSG_WriteByte( msg, ( pflags>>8 )&255 );
		MSG_WriteByte( msg, ( pflags>>16 )&255 );
		MSG_WriteByte( msg, ( pflags>>24 )&255 );
	}
	else if( pflags & 0x00ff0000 )
	{
		MSG_WriteByte( msg, ( pflags>>8 )&255 );
		MSG_WriteByte( msg, ( pflags>>16 )&255 );
	}
	else if( pflags & 0x0000ff00 )
	{
		MSG_WriteByte( msg, ( pflags>>8 )&255 );
	}

	// write the actual data

	if( pflags & FLPS_WEAPONSTATE )
		MSG_WriteByte( msg, playerState->weaponState );

	if( pflags & FLPS_DELTA_ANGLES0 )
		MSG_WriteShort( msg, playerState->delta_angles[0] );
	if( pflags & FLPS_DELTA_ANGLES1 )
		MSG_WriteShort( msg, playerState->delta_angles[1] );
	if( pflags & FLPS_DELTA_ANGLES2 )
		MSG_WriteShort( msg, playerState->delta_angles[2] );

	if( pflags & FLPS_MOVETIMERS )
	{
		for( i = 0; i < MAX_USERINPUT_STATS; i++ )
		{
			MSG_WriteShort( msg, playerState->controlTimers[i] );
		}
	}

	if( pflags & FLPS_HOLDABLELIST )
	{
		statbits = 0;
		for( i = 0; i < INV_MAX_SLOTS; i++ )
		{
			if( playerState->inventory[i][0] != deltaPlayerState->inventory[i][0] ||
			    playerState->inventory[i][1] != deltaPlayerState->inventory[i][1] )
				statbits |= ( 1<<i );
		}

		MSG_WriteShort( msg, statbits );
		for( i = 0; i < INV_MAX_SLOTS; i++ )
		{
			if( statbits & ( 1<<i ) )
			{
				MSG_WriteByte( msg, (qbyte)playerState->inventory[i][0] );
				MSG_WriteByte( msg, (qbyte)playerState->inventory[i][1] );
			}
		}
	}

	if( pflags & FLPS_EVENT )
	{
		if( !playerState->eventParm[0] )
			MSG_WriteByte( msg, playerState->event[0] & ~EV_INVERSE );
		else
		{
			MSG_WriteByte( msg, playerState->event[0] | EV_INVERSE );
			MSG_WriteByte( msg, playerState->eventParm[0] );
		}
	}

	if( pflags & FLPS_EVENT2 )
	{
		if( !playerState->eventParm[1] )
			MSG_WriteByte( msg, playerState->event[1] & ~EV_INVERSE );
		else
		{
			MSG_WriteByte( msg, playerState->event[1] | EV_INVERSE );
			MSG_WriteByte( msg, playerState->eventParm[1] );
		}
	}

	if( pflags & FLPS_POVNUM )
		MSG_WriteByte( msg, (qbyte)playerState->POVnum );

	if( pflags & FLPS_VIEWHEIGHT )
		MSG_WriteChar( msg, (char)playerState->viewHeight );

	if( pflags & FLPS_FOV )
		MSG_WriteByte( msg, (qbyte)playerState->fov );

	if( pflags & FLPS_VIEWTYPE )
		MSG_WriteByte( msg, (qbyte)playerState->viewType );

	// send stats
	statbits = 0;
	for( i = 0; i < PS_MAX_STATS; i++ )
	{
		if( playerState->stats[i] != deltaPlayerState->stats[i] )
			statbits |= 1<<i;
	}

	MSG_WriteLong( msg, statbits );
	for( i = 0; i < PS_MAX_STATS; i++ )
	{
		if( statbits & ( 1<<i ) )
			MSG_WriteShort( msg, playerState->stats[i] );
	}
}

//=================
//Com_ReadDeltaPlayerState
//=================
void Com_ReadDeltaPlayerState( msg_t *msg, player_state_t *deltaPlayerState, player_state_t *playerState )
{
	int flags;
	int i, b;
	int statbits;

	// copy over old into new
	memcpy( playerState, deltaPlayerState, sizeof( player_state_t ) );

	flags = (qbyte)MSG_ReadByte( msg );
	if( flags & FLPS_MOREBITS1 )
	{
		b = (qbyte)MSG_ReadByte( msg );
		flags |= b<<8;
	}
	if( flags & FLPS_MOREBITS2 )
	{
		b = (qbyte)MSG_ReadByte( msg );
		flags |= b<<16;
	}
	if( flags & FLPS_MOREBITS3 )
	{
		b = (qbyte)MSG_ReadByte( msg );
		flags |= b<<24;
	}

	if( flags & FLPS_WEAPONSTATE )
		playerState->weaponState = (qbyte)MSG_ReadByte( msg );

	if( flags & FLPS_DELTA_ANGLES0 )
		playerState->delta_angles[0] = MSG_ReadShort( msg );
	if( flags & FLPS_DELTA_ANGLES1 )
		playerState->delta_angles[1] = MSG_ReadShort( msg );
	if( flags & FLPS_DELTA_ANGLES2 )
		playerState->delta_angles[2] = MSG_ReadShort( msg );

	if( flags & FLPS_MOVETIMERS )
	{
		for( i = 0; i < MAX_USERINPUT_STATS; i++ )
			playerState->controlTimers[i] = MSG_ReadShort( msg );
	}

	if( flags & FLPS_HOLDABLELIST )
	{
		statbits = MSG_ReadShort( msg );
		for( i = 0; i < INV_MAX_SLOTS; i++ )
		{
			if( statbits & ( 1<<i ) )
			{
				playerState->inventory[i][0] = MSG_ReadByte( msg );
				playerState->inventory[i][1] = MSG_ReadByte( msg );
			}
		}
	}

	if( flags & FLPS_EVENT )
	{
		playerState->event[0] = MSG_ReadByte( msg );
		playerState->eventParm[0] = ( playerState->event[0] & EV_INVERSE ) ? MSG_ReadByte( msg ) : 0;
		playerState->event[0] &= ~EV_INVERSE;
	}
	else
	{
		playerState->event[0] = playerState->eventParm[0] = 0;
	}

	if( flags & FLPS_EVENT2 )
	{
		playerState->event[1] = MSG_ReadByte( msg );
		playerState->eventParm[1] = ( playerState->event[1] & EV_INVERSE ) ? MSG_ReadByte( msg ) : 0;
		playerState->event[1] &= ~EV_INVERSE;
	}
	else
	{
		playerState->event[1] = playerState->eventParm[1] = 0;
	}

	if( flags & FLPS_POVNUM )
		playerState->POVnum = (qbyte)MSG_ReadByte( msg );

	if( flags & FLPS_VIEWHEIGHT )
		playerState->viewHeight = MSG_ReadChar( msg );

	if( flags & FLPS_FOV )
		playerState->fov = (qbyte)MSG_ReadByte( msg );

	if( flags & FLPS_VIEWTYPE )
		playerState->viewType = (qbyte)MSG_ReadByte( msg );

	// parse stats
	statbits = MSG_ReadLong( msg );
	for( i = 0; i < PS_MAX_STATS; i++ )
	{
		if( statbits & ( 1<<i ) )
			playerState->stats[i] = MSG_ReadShort( msg );
	}
}

//==================================================
// entity_state_t communication
//==================================================

// if these are changed the game-code protocol must be changed.
// The ones using a U_HEADER_* macro can't be modified since they are used by the engine.

#define	FLES_MOVESTATE	    ( 1<<0 )
#define	FLES_FLAGS	    ( 1<<1 )
#define	FLES_EFFECTS8	    ( 1<<2 )
#define	FLES_WEAPON	    ( 1<<3 )
#define	FLES_EVENT	    ( 1<<4 )
#define	FLES_EVENT2	    ( 1<<5 )
#define	FLES_REMOVE	    U_HEADER_REMOVE         // REMOVE this entity, don't add it
#define	FLES_MOREBITS1	    U_HEADER_MOREBITS1      // read one additional byte

// second byte
#define	FLES_NUMBER16	    U_HEADER_NUMBER16       // NUMBER8 is implicit if not set
#define	FLES_EFFECTS16	    ( 1<<9 )
#define	FLES_SOLID	    ( 1<<10 )
#define	FLES_MODEL	    ( 1<<11 )
#define FLES_TYPE	    ( 1<<12 )
#define	FLES_TEAM_AND_CLASS ( 1<<13 )
#define FLES_SKIN8	    ( 1<<14 )
#define	FLES_MOREBITS2	    U_HEADER_MOREBITS2      // read one additional byte

// third byte
#define	FLES_OLDORIGIN	    ( 1<<16 )
#define FLES_MODEL2	    ( 1<<17 )
#define	FLES_SOUND	    ( 1<<18 )
#define	FLES_FREE_FOR_USE01	( 1<<19 )
#define	FLES_FREE_FOR_USE02	( 1<<20 )
#define	FLES_FREE_FOR_USE03	( 1<<21 )
#define	FLES_FREE_FOR_USE04	( 1<<22 )
#define	FLES_MOREBITS3	    U_HEADER_MOREBITS3      // read one additional byte

// fourth byte
#define	FLES_SKIN16		( 1<<24 )
#define	FLES_FREE_FOR_USE05	( 1<<25 )
#define FLES_FREE_FOR_USE06	( 1<<26 )
#define	FLES_FREE_FOR_USE07	( 1<<27 )
#define FLES_FREE_FOR_USE08	( 1<<28 )
#define	FLES_FREE_FOR_USE09	( 1<<29 )
#define	FLES_FREE_FOR_USE10	( 1<<30 )

//=================
//Com_EntityIsBaseLined
//=================
qboolean Com_EntityIsBaseLined( entity_state_t *state )
{
	return ( state->modelindex1 || state->sound ) ? qtrue : qfalse;
}

//=================
//Com_WriteDeltaEntityState
//=================
void Com_WriteDeltaEntityState( entity_state_t *from, entity_state_t *to, msg_t *msg, qboolean force )
{
	int bits;

	if( to->number < 0 || to->number >= MAX_EDICTS )
		Com_Error( ERR_DISCONNECT, "GS_WriteDeltaEntity: Invalid Entity number" );

	// send an update
	bits = 0;

	if( to->number & 0xFF00 )
		bits |= FLES_NUMBER16; // number8 is implicit otherwise

	if( Com_BitmaskDeltaMoveState( &from->ms, &to->ms ) )
	{
		bits |= FLES_MOVESTATE;
	}

	if( to->flags != from->flags )
	{
		bits |= FLES_FLAGS;
	}

	if( to->effects != from->effects )
	{
		if( to->effects & 0xFFFF0000 )
			bits |= ( FLES_EFFECTS8|FLES_EFFECTS16 );
		else if( to->effects & 0xFF00 )
			bits |= FLES_EFFECTS16;
		else
			bits |= FLES_EFFECTS8;
	}

	if( to->skinindex != from->skinindex )
	{
		if( to->skinindex & 0xFFFF0000 )
			bits |= ( FLES_SKIN8|FLES_SKIN16 );
		else if( to->skinindex & 0xFF00 )
			bits |= FLES_SKIN16;
		else
			bits |= FLES_SKIN8;
	}



	if( ( to->solid != from->solid ) || ( to->cmodeltype != from->cmodeltype ) || ( to->bbox != from->bbox ) )
		bits |= FLES_SOLID;

	// events are not delta compressed, just 0 compressed
	if( to->events[0] )
		bits |= FLES_EVENT;
	if( to->events[1] )
		bits |= FLES_EVENT2;

	if( to->modelindex1 != from->modelindex1 )
		bits |= FLES_MODEL;
	if( to->modelindex2 != from->modelindex2 )
		bits |= FLES_MODEL2;

	if( to->type != from->type )
		bits |= FLES_TYPE;

	if( to->weapon != from->weapon )
		bits |= FLES_WEAPON;

	if( ( to->team != from->team ) || ( to->playerclass != from->playerclass ) )
		bits |= FLES_TEAM_AND_CLASS;

	if( to->sound != from->sound )
		bits |= FLES_SOUND;

	if( to->origin2[0] != from->origin2[0]
	    || to->origin2[1] != from->origin2[1]
	    || to->origin2[2] != from->origin2[2] )
	{
		bits |= FLES_OLDORIGIN;
	}

	//------- write the message -------

	if( !bits && !force )
		return; // nothing to send!


	if( bits & 0xff000000 )
		bits |= FLES_MOREBITS3 | FLES_MOREBITS2 | FLES_MOREBITS1;
	else if( bits & 0x00ff0000 )
		bits |= FLES_MOREBITS2 | FLES_MOREBITS1;
	else if( bits & 0x0000ff00 )
		bits |= FLES_MOREBITS1;

	MSG_WriteByte( msg, bits&255 );

	if( bits & 0xff000000 )
	{
		MSG_WriteByte( msg, ( bits>>8 )&255 );
		MSG_WriteByte( msg, ( bits>>16 )&255 );
		MSG_WriteByte( msg, ( bits>>24 )&255 );
	}
	else if( bits & 0x00ff0000 )
	{
		MSG_WriteByte( msg, ( bits>>8 )&255 );
		MSG_WriteByte( msg, ( bits>>16 )&255 );
	}
	else if( bits & 0x0000ff00 )
	{
		MSG_WriteByte( msg, ( bits>>8 )&255 );
	}


	if( bits & FLES_NUMBER16 )
		MSG_WriteShort( msg, to->number );
	else
		MSG_WriteByte( msg, to->number );

	if( bits & FLES_MOVESTATE )
	{
		Com_WriteDeltaMoveState( msg, &from->ms, &to->ms );
	}

	if( bits & FLES_FLAGS )
	{
		MSG_WriteByte( msg, to->flags );
	}

	if( ( bits & ( FLES_EFFECTS8|FLES_EFFECTS16 ) ) == ( FLES_EFFECTS8|FLES_EFFECTS16 ) )
		MSG_WriteLong( msg, to->effects );
	else if( bits & FLES_EFFECTS8 )
		MSG_WriteByte( msg, to->effects );
	else if( bits & FLES_EFFECTS16 )
		MSG_WriteShort( msg, to->effects );

	if( bits & FLES_SOLID )
	{
		int bboxbit = ( to->bbox != from->bbox ) ? ET_INVERSE : 0;
		MSG_WriteByte( msg, ( ( to->solid&7 )|( to->cmodeltype&7 )<<4|bboxbit ) );
		if( bboxbit )
		{
			MSG_WriteShort( msg, to->bbox );
		}
	}

	if( bits & FLES_MODEL )
		MSG_WriteByte( msg, to->modelindex1 );
	if( bits & FLES_MODEL2 )
		MSG_WriteByte( msg, to->modelindex2 );

	if( ( bits & FLES_SKIN8 ) && ( bits & FLES_SKIN16 ) )  //used for laser colors
		MSG_WriteLong( msg, to->skinindex );
	else if( bits & FLES_SKIN8 )
		MSG_WriteByte( msg, to->skinindex );
	else if( bits & FLES_SKIN16 )
		MSG_WriteShort( msg, to->skinindex );

	if( bits & FLES_OLDORIGIN )
	{
		MSG_WriteCoord( msg, to->origin2[0] );
		MSG_WriteCoord( msg, to->origin2[1] );
		MSG_WriteCoord( msg, to->origin2[2] );
	}

	if( bits & FLES_TYPE )
	{
		MSG_WriteByte( msg, to->type );
	}

	if( bits & FLES_WEAPON )
	{
		MSG_WriteByte( msg, to->weapon & ( MAX_WEAPONS-1 ) );
	}

	if( bits & FLES_TEAM_AND_CLASS )
	{
		int team = ( to->team & ( MAX_TEAMS-1 ) );
		int pclass = ( to->playerclass & ( MAX_PLAYERCLASSES-1 ) )<<3;
		MSG_WriteByte( msg, ( team|pclass )&0xFF );
	}

	if( bits & FLES_SOUND )
		MSG_WriteByte( msg, (qbyte)to->sound );

	if( bits & FLES_EVENT )
	{
		if( !to->eventParms[0] )
		{
			MSG_WriteByte( msg, (qbyte)( to->events[0] & ~EV_INVERSE ) );
		}
		else
		{
			MSG_WriteByte( msg, (qbyte)( to->events[0]|EV_INVERSE ) );
			MSG_WriteByte( msg, (qbyte)to->eventParms[0] );
		}
	}
	if( bits & FLES_EVENT2 )
	{
		if( !to->eventParms[1] )
		{
			MSG_WriteByte( msg, (qbyte)( to->events[1] & ~EV_INVERSE ) );
		}
		else
		{
			MSG_WriteByte( msg, (qbyte)( to->events[1]|EV_INVERSE ) );
			MSG_WriteByte( msg, (qbyte)to->eventParms[1] );
		}
	}
}

//=================
//Com_ReadEntityBits
//=================
int Com_ReadEntityBits( msg_t *msg, unsigned *bits )
{
	unsigned b, total;
	int number;

	total = (qbyte)MSG_ReadByte( msg );
	if( total & U_HEADER_MOREBITS1 )
	{
		b = (qbyte)MSG_ReadByte( msg );
		total |= ( b<<8 )&0x0000FF00;
	}
	if( total & U_HEADER_MOREBITS2 )
	{
		b = (qbyte)MSG_ReadByte( msg );
		total |= ( b<<16 )&0x00FF0000;
	}
	if( total & U_HEADER_MOREBITS3 )
	{
		b = (qbyte)MSG_ReadByte( msg );
		total |= ( b<<24 )&0xFF000000;
	}

	if( total & U_HEADER_NUMBER16 )
		number = MSG_ReadShort( msg );
	else
		number = (qbyte)MSG_ReadByte( msg );

	*bits = total;

	return number;
}

//=================
//Com_ReadDeltaEntityState
//=================
void Com_ReadDeltaEntityState( msg_t *msg, entity_state_t *from, entity_state_t *to, int number, unsigned bits )
{
	// set old as base
	*to = *from;

	to->number = number;

	if( bits & FLES_MOVESTATE )
	{
		Com_ReadDeltaMoveState( msg, &from->ms, &to->ms );
	}

	if( bits & FLES_FLAGS )
	{
		to->flags = (qbyte)MSG_ReadByte( msg );
	}

	if( ( bits & ( FLES_EFFECTS8|FLES_EFFECTS16 ) ) == ( FLES_EFFECTS8|FLES_EFFECTS16 ) )
		to->effects = MSG_ReadLong( msg );
	else if( bits & FLES_EFFECTS8 )
		to->effects = (qbyte)MSG_ReadByte( msg );
	else if( bits & FLES_EFFECTS16 )
		to->effects = MSG_ReadShort( msg );

	if( bits & FLES_SOLID )
	{
		to->solid = (qbyte)MSG_ReadByte( msg );
		if( to->solid & ET_INVERSE )
		{
			to->bbox = MSG_ReadShort( msg );
		}
		to->solid &= ~ET_INVERSE;
		to->cmodeltype = ( ( to->solid>>4 )&7 );
		to->solid = to->solid & 7;
	}

	if( bits & FLES_MODEL )
		to->modelindex1 = (qbyte)MSG_ReadByte( msg );
	if( bits & FLES_MODEL2 )
		to->modelindex2 = (qbyte)MSG_ReadByte( msg );

	if( ( bits & FLES_SKIN8 ) && ( bits & FLES_SKIN16 ) )
		to->skinindex = MSG_ReadLong( msg );
	else if( bits & FLES_SKIN8 )
		to->skinindex = MSG_ReadByte( msg );
	else if( bits & FLES_SKIN16 )
		to->skinindex = MSG_ReadShort( msg );

	if( bits & FLES_OLDORIGIN )
		MSG_ReadPos( msg, to->origin2 );

	if( bits & FLES_TYPE )
	{
		to->type = (qbyte)MSG_ReadByte( msg );
	}

	if( bits & FLES_WEAPON )
	{
		to->weapon = (qbyte)( MSG_ReadByte( msg ) & (MAX_WEAPONS-1) );
	}

	if( bits & FLES_TEAM_AND_CLASS )
	{
		qbyte dat;
		dat = (qbyte)MSG_ReadByte( msg );
		to->team = ( dat & ( MAX_TEAMS-1 ) );
		to->playerclass = ( dat>>3 & ( MAX_PLAYERCLASSES-1 ) );
	}

	if( bits & FLES_SOUND )
		to->sound = (qbyte)MSG_ReadByte( msg );

	if( bits & FLES_EVENT )
	{
		int event;
		event = (qbyte)MSG_ReadByte( msg );
		if( event & EV_INVERSE )
		{
			to->eventParms[0] = (qbyte)MSG_ReadByte( msg );
		}
		else
		{
			to->eventParms[0] = 0;
		}
		to->events[0] = ( event & ~EV_INVERSE );
	}
	else
	{
		to->events[0] = 0;
		to->eventParms[0] = 0;
	}

	if( bits & FLES_EVENT2 )
	{
		int event;
		event = (qbyte)MSG_ReadByte( msg );
		if( event & EV_INVERSE )
		{
			to->eventParms[1] = (qbyte)MSG_ReadByte( msg );
		}
		else
		{
			to->eventParms[1] = 0;
		}
		to->events[1] = ( event & ~EV_INVERSE );
	}
	else
	{
		to->events[1] = 0;
		to->eventParms[1] = 0;
	}
}
