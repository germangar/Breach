/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

cgame_media_t cgm;

//==================================================
// LOCAL MODELS
//==================================================

typedef struct cg_localmodel_s
{
	char *name;
	int index;
	qboolean precache;
	void *data;
} cg_localmodel_t;

cg_localmodel_t cg_localModels[] =
{
	{ "models/weapons/rlauncher/flash.md3", MODEL_TESTIMPACT, qtrue, NULL },
	{ NULL }
};

//=================
//CG_PrecacheLocalModels
//=================
void CG_PrecacheLocalModels( void )
{
	cg_localmodel_t *localmodel;
	int i;

	for( i = 0, localmodel = cg_localModels; localmodel->name; localmodel++, i++ )
	{
		if( i != localmodel->index )
			GS_Error( "CG_RegisterLocalModels: Invalid localmodel index %i != %i\n", i, localmodel->index );

		if( localmodel->precache )
		{
			localmodel->data = ( void * )CG_RegisterModel( localmodel->name );
		}
	}
}

//=================
//CG_LocalModel
//=================
struct model_s *CG_LocalModel( int index )
{
	cg_localmodel_t *localmodel;

	if( index < 0 || index >= LM_TOTAL_MODELS )
		return NULL;

	localmodel = &cg_localModels[index];
	if( !localmodel->data )
		localmodel->data = ( void * )CG_RegisterModel( localmodel->name );

	return ( struct model_s * )localmodel->data;
}

//==================================================
// LOCAL SOUNDS
//==================================================

typedef struct cg_localsound_s
{
	char *name;
	int index;
	qboolean precache;
	void *data;
} cg_localsound_t;

cg_localsound_t cg_localSounds[] =
{
	{ "sounds/misc/chat", SOUND_CHAT, qtrue, NULL },
	{ "sounds/weapons/weaponswitch", SOUND_WEAPONSWITCH, qtrue, NULL },
	{ "sounds/weapons/noammo", SOUND_NOAMMOCLICK, qtrue, NULL },
	{ "sounds/world/water_splash", SOUND_WATERSPLASH, qtrue, NULL },
	{ "sounds/world/water_in", SOUND_WATERENTER, qtrue, NULL },
	{ "sounds/world/water_out", SOUND_WATEREXIT, qtrue, NULL },

	// player pickups
	{ "sounds/players/pickup_weapon", SOUND_PICKUP_WEAPON, qtrue, NULL },
	{ "sounds/players/pickup_ammo", SOUND_PICKUP_AMMO, qtrue, NULL },
	{ "sounds/players/pickup_health", SOUND_PICKUP_HEALTH, qtrue, NULL },

	// footsteps
	{ "sounds/players/footstep1", SOUND_FOOTSTEPS, qtrue, NULL },
	{ "sounds/players/footstepmetal1", SOUND_FOOTSTEPS_METAL, qtrue, NULL },
	{ "sounds/players/footstepwater1", SOUND_FOOTSTEPS_WATER, qtrue, NULL },
	{ "sounds/players/footstepdust1", SOUND_FOOTSTEPS_DUST, qtrue, NULL },

	// mover events
	{ "sounds/world/movers/button_activate", SOUND_BUTTON_ACTIVATE, qtrue, NULL },
	{ "sounds/world/movers/plat_start", SOUND_PLAT_START, qtrue, NULL },
	{ "sounds/world/movers/plat_stop", SOUND_PLAT_STOP, qtrue, NULL },

	// announcer
	{ "sounds/announcer/countdown/01", SOUND_COUNTDOWN_01, qtrue, NULL },
	{ "sounds/announcer/countdown/02", SOUND_COUNTDOWN_02, qtrue, NULL },
	{ "sounds/announcer/countdown/03", SOUND_COUNTDOWN_03, qtrue, NULL },

	{ NULL }
};

//=================
//CG_PrecacheLocalSounds
//=================
void CG_PrecacheLocalSounds( void )
{
	cg_localsound_t *localsound;
	int i;

	for( i = 0, localsound = cg_localSounds; localsound->name; localsound++, i++ )
	{
		if( i != localsound->index )
			GS_Error( "CG_RegisterLocalSounds: Invalid localsound index %i != %i\n", i, localsound->index );

		if( localsound->precache )
		{
			localsound->data = ( void * )trap_S_RegisterSound( localsound->name );
		}
	}
}

//=================
//CG_LocalSound
//=================
struct sfx_s *CG_LocalSound( int index )
{
	cg_localsound_t *localsound;

	if( index < 0 || index >= LM_TOTAL_SOUNDS )
		return NULL;

	localsound = &cg_localSounds[index];
	if( !localsound->data )
		localsound->data = ( void * )trap_S_RegisterSound( localsound->name );

	return ( struct sfx_s * )localsound->data;
}

//==================================================
// LOCAL SHADERS
//==================================================

typedef struct cg_localshader_s
{
	char *name;
	int index;
	qboolean precache;
	void *data;
} cg_localshader_t;

cg_localshader_t cg_localShaders[] =
{
	{ "2d/white", SHADER_WHITE, qtrue, NULL },
	{ "gfx/2d/bubblechat", SHADER_CHAT, qtrue, NULL },
	{ "gfx/misc/shadow", SHADER_PLAYERSHADOW, qtrue, NULL },
	{ "gfx/misc/colorlaser", SHADER_LINEBEAM, qfalse, NULL },
	{ "2d/ui/cursor", SHADER_CURSOR, qtrue, NULL },
	{ "2d/hud/crosshairs/000", SHADER_CROSSHAIR, qtrue, NULL },

	{ "gfx/explosion1", SHADER_EXPLOSION_ONE, qtrue, NULL },

	{ NULL }
};

//=================
//CG_PrecacheLocalShaders
//=================
void CG_PrecacheLocalShaders( void )
{
	cg_localshader_t *localshader;
	int i;

	for( i = 0, localshader = cg_localShaders; localshader->name; localshader++, i++ )
	{
		if( i != localshader->index )
			GS_Error( "CG_RegisterLocalShaders: Invalid localshader index %i != %i\n", i, localshader->index );

		if( localshader->precache )
		{
			localshader->data = ( void * )trap_R_RegisterPic( localshader->name );
		}
	}
}

//=================
//CG_LocalShader
//=================
struct shader_s *CG_LocalShader( int index )
{
	cg_localshader_t *localshader;

	if( index < 0 || index >= LM_TOTAL_SHADERS )
		return NULL;

	localshader = &cg_localShaders[index];
	if( !localshader->data )
		localshader->data = ( void * )trap_R_RegisterPic( localshader->name );

	return ( struct shader_s * )localshader->data;
}

//==================================================
// FONTS
//==================================================

//=================
//CG_RegisterFonts
//=================
void CG_RegisterFonts( void )
{
	cvar_t *con_fontSystemSmall = trap_Cvar_Get( "con_fontSystemSmall", DEFAULT_FONT_SMALL, CVAR_ARCHIVE );
	cvar_t *con_fontSystemMedium = trap_Cvar_Get( "con_fontSystemMedium", DEFAULT_FONT_MEDIUM, CVAR_ARCHIVE );
	cvar_t *con_fontSystemBig = trap_Cvar_Get( "con_fontSystemBig", DEFAULT_FONT_BIG, CVAR_ARCHIVE );

	cgm.fontSystemSmall = trap_SCR_RegisterFont( con_fontSystemSmall->string );
	if( !cgm.fontSystemSmall )
	{
		cgm.fontSystemSmall = trap_SCR_RegisterFont( DEFAULT_FONT_SMALL );
		if( !cgm.fontSystemSmall )
			GS_Error( "Couldn't load default font \"%s\"", DEFAULT_FONT_SMALL );
	}
	cgm.fontSystemMedium = trap_SCR_RegisterFont( con_fontSystemMedium->string );
	if( !cgm.fontSystemMedium )
		cgm.fontSystemMedium = trap_SCR_RegisterFont( DEFAULT_FONT_MEDIUM );

	cgm.fontSystemBig = trap_SCR_RegisterFont( con_fontSystemBig->string );
	if( !cgm.fontSystemBig )
		cgm.fontSystemBig = trap_SCR_RegisterFont( DEFAULT_FONT_BIG );
}
