/*
   Copyright (C) 2007 German Garcia
 */

enum
{
	SOUND_NOSOUND
	, SOUND_HIT_WATER

	, SOUND_PLAT_MOVE
	, SOUND_DOOR_MOVE
	, SOUND_DOOR_ROTATING_MOVE
	, SOUND_ROTATING_MOVE

	, LM_TOTAL_SOUNDS
};

extern int G_LocalSound( int index );
extern void G_PrecacheLocalSounds( void );
extern char *G_LocalSoundName( int index );
