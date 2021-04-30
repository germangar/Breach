/*
Copyright (C) 2007 German Garcia
*/

#include "cg_local.h"

/*
* CG_ForcedTeamColor
*/
int CG_ForcedTeamColor( int team )
{
	int rgbacolor;
	assert( team >= 0 && team < GS_NUMTEAMS );
	rgbacolor = GS_TeamColor( team );

	// add client side forcing override

	return rgbacolor;
}

/*
* CG_SetSceneTeamColors
* Updates the team colors in the renderer with the ones assigned to each team.
*/
void CG_SetSceneTeamColors( void )
{
	int rgbacolor;

	rgbacolor = CG_ForcedTeamColor( TEAM_NOTEAM );
	trap_R_SetCustomColor( TEAM_NOTEAM, COLOR_R( rgbacolor ), COLOR_G( rgbacolor ), COLOR_B( rgbacolor ) );

	rgbacolor = CG_ForcedTeamColor( TEAM_SPECTATOR );
	trap_R_SetCustomColor( TEAM_SPECTATOR, COLOR_R( rgbacolor ), COLOR_G( rgbacolor ), COLOR_B( rgbacolor ) );
}

/*
* CG_PModelForCentity
*/
playerobject_t *CG_PModelForCentity( centity_t *cent )
{
	if( cent->current.playerclass )
	{
		gsplayerclass_t *playerClass = GS_PlayerClassByIndex( cent->current.playerclass );

		if( playerClass && playerClass->objectIndex )
			return cgm.indexedPlayerObjects[playerClass->objectIndex];
	}

	return cgm.indexedPlayerObjects[cent->current.modelindex1]; // return server defined one
}

/*
* CG_SkinForCentity
*/
struct skinfile_s *CG_SkinForCentity( centity_t *cent )
{
	return cgm.indexedSkins[cent->current.skinindex]; // return server defined one
}
