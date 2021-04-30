/*
   Copyright (C) 2007 German Garcia
 */

extern cvar_t *cg_volume_players;
extern cvar_t *cg_volume_effects;
extern cvar_t *cg_volume_hitsound;
extern cvar_t *cg_volume_voicechats;

extern void CG_InitSound( void );
extern void CG_FixVolumeCvars( void );
extern void CG_AddAnnouncerEvent( struct sfx_s *sound, qboolean queued );
extern void CG_AddLocalSceneSounds( void );
extern void CG_StartGlobalIndexedSound( int soundindex, int entChannel, float fvol );
extern void CG_StartRelativeIndexedSound( int soundindex, int entNum, int entChannel, float fvol, float attenuation );
extern void CG_StartFixedIndexedSound( int soundindex, vec3_t origin, int entChannel, float fvol, float attenuation );
extern void CG_GetEntitySpatilization( int entNum, vec3_t origin, vec3_t velocity );
extern void CG_StartBackgroundTrack( void );
