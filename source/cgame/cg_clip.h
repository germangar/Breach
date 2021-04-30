/*
   Copyright (C) 2007 German Garcia
 */

extern void CG_Clip_UnlinkAllEntities( void );
extern void CG_Clip_LinkEntityState( entity_state_t *state );
extern entity_state_t *CG_GetClipStateForDeltaTime( int entNum, int deltaTime );
