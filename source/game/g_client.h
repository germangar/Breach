/*
   Copyright (C) 2007 German Garcia
 */

#ifndef __G_CLIENT_H__
#define __G_CLIENT_H__

extern qboolean G_ClientConnect( int clientNum, char *userinfo, qboolean fakeClient );
extern void G_ClientDisconnect( int clientNum );
extern void G_ClientUpdateInfoString( gclient_t *client );
extern void G_ClientUserinfoChanged( int clientNum, char *userinfo );
extern gentity_t *G_SpawnFakeClient( const char *name );
extern void G_WriteDeltaPlayerState( int clientNum, unsigned int deltaSnapNum, unsigned int snapNum, msg_t *msg );
extern void G_ClientCloseSnap( gentity_t *ent );
extern void G_ClientBegin( int clientNum );
extern qboolean G_Client_Respawn( gentity_t *ent, qboolean ghost );
extern void G_ClientDie( gentity_t *ent );
extern void G_Client_Activate( gentity_t *ent, int targetNum );
extern void G_ClientThink( int clientNum, usercmd_t *cmd, int timeDelta );

//et_player

extern void G_et_player_spawn( gentity_t *ent, vec3_t spawn_origin, vec3_t spawn_angles, char *playerObject );

#endif // __G_CLIENT_H__
