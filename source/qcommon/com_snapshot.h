
#ifndef _COM_SNAPSHOT_
#define _COM_SNAPSHOT_

extern entity_state_t nullEntityState;
extern player_state_t nullPlayerState;
extern game_state_t nullGameState;

//==================================================
// game_state_t communication
//==================================================
extern void Com_WriteDeltaGameState( msg_t *msg, game_state_t *deltaGameState, game_state_t *gameState );
extern void Com_ReadDeltaGameState( msg_t *msg, game_state_t *deltaGameState, game_state_t *gameState );

//==================================================
// move_state_t communication
//==================================================

extern unsigned int Com_BitmaskDeltaMoveState( move_state_t *deltaMoveState, move_state_t *moveState );
extern unsigned int Com_WriteDeltaMoveState( msg_t *msg, move_state_t *deltaMoveState, move_state_t *moveState );
extern void Com_ReadDeltaMoveState( msg_t *msg, move_state_t *deltaMoveState, move_state_t *moveState );

//==================================================
// player_state_t communication
//==================================================

extern void Com_WriteDeltaPlayerstate( msg_t *msg, player_state_t *deltaPlayerState, player_state_t *playerState );
extern void Com_ReadDeltaPlayerState( msg_t *msg, player_state_t *deltaPlayerState, player_state_t *playerState );

//==================================================
// entity_state_t communication
//==================================================
extern qboolean Com_EntityIsBaseLined( entity_state_t *state );
extern void Com_WriteDeltaEntityState( entity_state_t *from, entity_state_t *to, msg_t *msg, qboolean force );
extern int Com_ReadEntityBits( msg_t *msg, unsigned *bits );
extern void Com_ReadDeltaEntityState( msg_t *msg, entity_state_t *from, entity_state_t *to, int number, unsigned bits );

#endif // -_COM_SNAPSHOT_
