/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

cvar_t *cg_volume_players;
cvar_t *cg_volume_effects;
cvar_t *cg_volume_hitsound;
cvar_t *cg_volume_voicechats;
cvar_t *cg_volume_announcer;

//=================
//CG_InitSound
//=================
void CG_InitSound( void )
{
	cg_volume_players =	trap_Cvar_Get( "cg_volume_players", "1.0", CVAR_ARCHIVE );
	cg_volume_effects =	trap_Cvar_Get( "cg_volume_effects", "1.0", CVAR_ARCHIVE );
	cg_volume_hitsound =	trap_Cvar_Get( "cg_volume_hitsound", "1.0", CVAR_ARCHIVE );
	cg_volume_voicechats =	trap_Cvar_Get( "cg_volume_voicechats", "1.0", CVAR_ARCHIVE );
	cg_volume_announcer =	trap_Cvar_Get( "cg_volume_announcer", "1.0", CVAR_ARCHIVE );
}

//=================
//CG_FixVolumeCvars
//=================
void CG_FixVolumeCvars( void )
{
	if( cg_volume_players->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_players", 0.0f );
	else if( cg_volume_players->value > 2.0f )
		trap_Cvar_SetValue( "cg_volume_players", 2.0f );

	if( cg_volume_effects->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_effects", 0.0f );
	else if( cg_volume_effects->value > 2.0f )
		trap_Cvar_SetValue( "cg_volume_effects", 2.0f );

	if( cg_volume_hitsound->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_hitsound", 0.0f );
	else if( cg_volume_hitsound->value > 10.0f )
		trap_Cvar_SetValue( "cg_volume_hitsound", 10.0f );
}

//=================
//CG_GetEntitySpatilization
//Called to get the sound spatialization origin and velocity
//=================
void CG_GetEntitySpatilization( int entNum, vec3_t origin, vec3_t velocity )
{
	centity_t *cent;

	if( entNum < -1 || entNum >= MAX_EDICTS )
		GS_Error( "CG_GetEntitySoundOrigin: bad entnum" );

	// hack for client side floatcam
	if( entNum == -1 )
		entNum = cgs.playerNum;

	cent = &cg_entities[entNum];

	if( origin != NULL )
		GS_CenterOfEntity( cent->ent.origin, &cent->current, origin );

	if( velocity != NULL )
		VectorCopy( cent->current.ms.velocity, velocity );
}

//=================
//CG_StartGlobalIndexedSound
//=================
void CG_StartGlobalIndexedSound( int soundindex, int entChannel, float fvol )
{

	// sexed sounds are not in the sound index and ignore attenuation
	if( cgm.indexedSounds[soundindex] )
	{
		trap_S_StartGlobalSound( cgm.indexedSounds[soundindex], entChannel, fvol );
	}
}

//=================
//CG_StartRelativeIndexedSound
//=================
void CG_StartRelativeIndexedSound( int soundindex, int ownerNum, int entChannel, float fvol, float attenuation )
{
	if( ownerNum < 0 || ownerNum >= MAX_EDICTS )
		GS_Error( "CG_StartRelativeSound: bad entnum" );

	// sexed sounds are not in the sound index and ignore attenuation
	if( !cgm.indexedSounds[soundindex] )
	{
		char *cstring = (char *)trap_GetConfigString( CS_SOUNDS + soundindex );
		if( cstring && cstring[0] == '*' )
		{
			CG_SexedSound( ownerNum, entChannel, cstring, 1.0 );
		}
		return;
	}

	if( ISVIEWERENTITY( ownerNum ) )
		trap_S_StartGlobalSound( cgm.indexedSounds[soundindex], entChannel, fvol );
	else
	{
		trap_S_StartRelativeSound( cgm.indexedSounds[soundindex], ownerNum, entChannel, fvol, attenuation );
	}
}

//=================
//CG_StartFixedIndexedSound
//=================
void CG_StartFixedIndexedSound( int soundindex, vec3_t origin, int entChannel, float fvol, float attenuation )
{

	// sexed sounds are not in the sound index and ignore attenuation
	if( cgm.indexedSounds[soundindex] )
	{
		if( origin == NULL )
			trap_S_StartGlobalSound( cgm.indexedSounds[soundindex], entChannel, fvol );
		else
			trap_S_StartFixedSound( cgm.indexedSounds[soundindex], origin, entChannel, fvol, attenuation );
	}
}

//=================
//CG_StartBackgroundTrack
//=================
void CG_StartBackgroundTrack( void )
{
	const char *string;
	char intro[MAX_QPATH], loop[MAX_QPATH];

	string = trap_GetConfigString( CS_AUDIOTRACK );
	Q_strncpyz( intro, COM_Parse( &string ), sizeof( intro ) );
	Q_strncpyz( loop, COM_Parse( &string ), sizeof( loop ) );

	trap_S_StartBackgroundTrack( intro, loop );
}

//==================================================
// ANNOUNCER SOUNDS
//==================================================

#define CG_ANNOUNCER_EVENTS_FRAMETIME 1.5 * 1000 // the announcer will speak each 1.5 seconds

/*
* CG_ClearAnnouncerEvents
*/
void CG_ClearAnnouncerEvents( void )
{
	cg.announcer.eventsCurrent = cg.announcer.eventsHead = 0;
}

/*
* CG_AddAnnouncerEvent
*/
void CG_AddAnnouncerEvent( struct sfx_s *sound, qboolean queued )
{
	if( !sound )
		return;

	if( !queued )
	{
		trap_S_StartGlobalSound( sound, CHAN_AUTO, cg_volume_announcer->value );
		cg.announcer.eventsDelay = CG_ANNOUNCER_EVENTS_FRAMETIME; // wait
		return;
	}

	if( cg.announcer.eventsCurrent + CG_MAX_ANNOUNCER_EVENTS >= cg.announcer.eventsHead )
	{
		// full buffer (we do nothing, just let it overwrite the oldest)
	}

	// add it
	cg.announcer.sounds[cg.announcer.eventsHead & CG_MAX_ANNOUNCER_EVENTS_MASK] = sound;
	cg.announcer.eventsHead++;
}

/*
* CG_ReleaseAnnouncerEvents
*/
void CG_ReleaseAnnouncerEvents( void )
{
	// see if enough time has passed
	cg.announcer.eventsDelay -= cg.frametime * 1000;
	if( cg.announcer.eventsDelay > 0 )
		return;

	if( cg.announcer.eventsCurrent < cg.announcer.eventsHead )
	{
		struct sfx_s *sound;

		// play the event
		sound = cg.announcer.sounds[cg.announcer.eventsCurrent & CG_MAX_ANNOUNCER_EVENTS_MASK];
		if( sound )
		{
			trap_S_StartGlobalSound( sound, CHAN_AUTO, cg_volume_announcer->value );
			cg.announcer.eventsDelay = CG_ANNOUNCER_EVENTS_FRAMETIME; // wait
		}
		cg.announcer.eventsCurrent++;
	}
	else
	{
		cg.announcer.eventsDelay = 0; // no wait
	}
}

//==================================================
// SCENE SOUNDS
//==================================================

//=================
//CG_AddLocalSceneSounds
//=================
void CG_AddLocalSceneSounds( void )
{
	static unsigned int lastSecond = 0;

	// count down sounds
	
	// add local announces
	if( GS_Countdown() )
	{
		if( GS_MatchDuration() )
		{
			unsigned int duration, curtime, remainingSeconds;
			float seconds;

			curtime = GS_Paused() ? cg.frame.timeStamp : cg.time;
			duration = GS_MatchDuration();

			if( duration + GS_MatchStartTime() < curtime ) 
				duration = curtime - GS_MatchStartTime(); // avoid negative results

			seconds = (float)( GS_MatchStartTime() + duration - curtime ) * 0.001f;
			remainingSeconds = (unsigned int)seconds;

			if( remainingSeconds != lastSecond )
			{
				if( 1 + remainingSeconds < 4 )
				{
					CG_AddAnnouncerEvent( CG_LocalSound( SOUND_COUNTDOWN_01 + remainingSeconds ), qfalse );
				}

				lastSecond = remainingSeconds;
			}
		}
	}
	else
		lastSecond = 0;

	// add sounds from announcer queue
	CG_ReleaseAnnouncerEvents();
}
