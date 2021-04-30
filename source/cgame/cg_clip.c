/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

entity_state_t *CG_GetClipStateForDeltaTime( int entNum, int deltaTime )
{
	centity_t *cent;
	assert( entNum >= 0 && entNum < MAX_EDICTS );
	cent = &cg_entities[entNum];

	//if( cent->snapNum != cg.frame.snapNum ) // was removed (not needed, if not in snap it's not linked)
	//	return NULL;

	return &cg_entities[entNum].current;
}

static int cg_linkedEntities[MAX_EDICTS];
static int cg_numLinkedEntities = 0;

void CG_Clip_LinkEntityState( entity_state_t *state )
{
	if( GS_SpaceLinkEntity( state ) )
	{
		cg_linkedEntities[cg_numLinkedEntities] = state->number;
		cg_numLinkedEntities++;
	}
}

void CG_Clip_UnlinkAllEntities( void )
{
	int i;
	for( i = 0; i < cg_numLinkedEntities; i++ )
	{
		GS_SpaceUnLinkEntity( &cg_entities[cg_linkedEntities[i]].current );
	}

	cg_numLinkedEntities = 0;
}
