/*
   Copyright (C) 2007 German Garcia
 */

#include "g_local.h"

//==================================================
// LOCAL SOUNDS
//==================================================

typedef struct g_localsound_s
{
	char *name;
	int index;
	qboolean precache;
	qboolean pure;
	int handle;
} g_localsound_t;

g_localsound_t g_localSounds[] =
{
	{ "sounds/nosound", SOUND_NOSOUND, qtrue, qfalse, 0 },
	{ "sounds/misc/hit_water", SOUND_HIT_WATER, qtrue, qtrue, 0 },

	// mover sounds are precache by the entity at spawning
	{ "sounds/world/movers/plat_move", SOUND_PLAT_MOVE, qfalse, qtrue, 0 },
	{ "sounds/world/movers/door_move", SOUND_DOOR_MOVE, qfalse, qtrue, 0 },
	{ "sounds/world/movers/door_rotating_move", SOUND_DOOR_ROTATING_MOVE, qfalse, qtrue, 0 },
	{ "sounds/world/movers/elevator_move", SOUND_ROTATING_MOVE, qfalse, qtrue, 0 },

	{ NULL }
};

//=================
//G_PrecacheLocalSounds
//=================
void G_PrecacheLocalSounds( void )
{
	g_localsound_t *localsound;
	int i;

	for( i = 0, localsound = g_localSounds; localsound->name; localsound++, i++ )
	{
		if( i != localsound->index )
			GS_Error( "G_PrecacheLocalSounds: Invalid localsound index %i != %i\n", i, localsound->index );

		if( localsound->precache )
			localsound->handle = trap_SoundIndex( localsound->name );

		if( localsound->pure )
			trap_PureSound( localsound->name );
	}
}

//=================
//G_LocalSound
//=================
int G_LocalSound( int index )
{
	g_localsound_t *localsound;

	if( index < 0 || index >= LM_TOTAL_SOUNDS )
		GS_Error( "G_LocalSound: Invalid localsound index %i\n", index );

	localsound = &g_localSounds[index];
	if( !localsound->handle )
		localsound->handle = trap_SoundIndex( localsound->name );

	return localsound->handle;
}

//=================
//G_LocalSoundName
//=================
char *G_LocalSoundName( int index )
{
	g_localsound_t *localsound;

	if( index < 0 || index >= LM_TOTAL_SOUNDS )
		GS_Error( "G_LocalSound: Invalid localsound index %i\n", index );

	localsound = &g_localSounds[index];

	return localsound->name;
}
