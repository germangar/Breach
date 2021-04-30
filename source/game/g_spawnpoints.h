/*
Copyright (C) 2007 German Garcia
*/

#ifndef __G_SPAWNPOINTS_H__
#define __G_SPAWNPOINTS_H__

enum 
{
	SPAWNSYSTEM_INSTANT,
	SPAWNSYSTEM_WAVES,
	SPAWNSYSTEM_HOLD,
};

extern qboolean G_SelectClientSpawnPoint( gentity_t *ent, int team, vec3_t spawn_origin, vec3_t spawn_angles );
extern void G_SpawnQueue_Init( void );
extern void G_SpawnQueue_AddClient( gentity_t *ent );
extern void G_SpawnQueue_Think( void );
extern int G_SpawnQueue_NextRespawnTime( int team );

#endif // __G_SPAWNPOINTS_H__
