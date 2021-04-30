/*
Copyright (C) 2007 German Garcia
*/

#include "g_local.h"

//======================================================
//		Match state machine
//======================================================

/*
* G_Gametype_SetMatchState
*/
void G_Gametype_SetMatchState( int matchState )
{
	int minutes = 20;

	switch( matchState )
	{
	default:
	case MATCH_STATE_PLAYTIME:
		{
			gs.gameState.stats[GAMESTAT_MATCHSTATE] = MATCH_STATE_PLAYTIME;
			gs.gameState.longstats[GAMELONG_MATCHSTART] = game.serverTime;
			gs.gameState.longstats[GAMELONG_MATCHDURATION] = 1000 * 60 * minutes;
		}
		break;

	}
}

/*
* G_Gametype_AdvanceMatchState
*/
void G_Gametype_AdvanceMatchState( void )
{
	unsigned int startTime;
	unsigned int duration;

	if( !gs.gameState.stats[GAMESTAT_MATCHSTATE] )
		G_Gametype_SetMatchState( MATCH_STATE_PLAYTIME );

	startTime = gs.gameState.longstats[GAMELONG_MATCHSTART];
	duration = gs.gameState.longstats[GAMELONG_MATCHDURATION];

	if( duration && ( startTime + duration > game.serverTime ) )
		return;

	// this state time limit is reached
	G_Gametype_SetMatchState( gs.gameState.stats[GAMESTAT_MATCHSTATE] + 1 );
}

cvar_t *g_votable_gametypes;

//======================================================
//		Game types
//======================================================

/*
* G_Gametype_Init
*/
void G_Gametype_Init( void )
{
	cvar_t *g_gametype;

	// empty string to allow all
	g_votable_gametypes =	trap_Cvar_Get( "g_votable_gametypes", "", CVAR_ARCHIVE );

	// the gametype cvar is only read at level initialization
	g_gametype = trap_Cvar_Get( "g_gametype", "breach", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_LATCH );
	
	if( g_gametype->latched_string )
		trap_Cvar_ForceSet( "g_gametype", g_gametype->latched_string );

	g_gametype->modified = qfalse;

	level.gametype.spawnableItemsMask = IT_WEAPON|IT_AMMO|IT_HEALTH;
	GS_GamestatSetFlag( GAMESTAT_FLAG_TEAMBASED, qtrue );

	if( !G_asLoadGametypeScript( g_gametype->string ) )
	{
	}
}

/*
* G_Gametype_CanPickUpItem
*/
qboolean G_Gametype_CanPickUpItem( gsitem_t *item )
{
	return qtrue;
}

/*
* G_Gametype_CanRespawnItem
*/
qboolean G_Gametype_CanRespawnItem( gsitem_t *item )
{
	return qfalse;
}

/*
* G_Gametype_CanDropItem
*/
qboolean G_Gametype_CanDropItem( gsitem_t *item )
{
	return qfalse;
}
